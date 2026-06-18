#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <memory>
#include <string>

/**
 * @brief 用 C++ 标准库实现的「同步阻塞 RPC stub」最小演示
 *
 * 三个角色：
 *   1. Client Stub (CalculatorStub) —— 把远程调用伪装成本地同步函数调用
 *   2. Server Stub (CalculatorServer::dispatch) —— 解包请求并路由到真实实现
 *   3. 传输层 (Channel) —— 这里用内存队列模拟网络，真实场景是 socket
 *
 * 同步阻塞的原理：
 *   - client 端：fut.get() 让调用线程睡眠，直到 server 写入结果才被唤醒
 *   - server 端：m_cv.wait() 让工作线程睡眠，直到有请求到达才被唤醒
 *   - promise / future 是打通两个线程的「请求-应答 + 阻塞-唤醒」原语
 */

// ============ 模拟“网络传输层” ============
// 一个请求 = 方法名 + 参数 + 一个用来回填结果的 promise
struct Request
{
    std::string       method;
    int               a, b;
    std::promise<int> result;   // server 通过它把结果送回 client
};

class Channel
{                 // 模拟网络通道(请求队列)
public:
    void send( std::shared_ptr<Request> req )
    {
        {
            std::lock_guard<std::mutex> lk( m_mtx );
            m_queue.push( req );
        }
        m_cv.notify_one( );
    }
    std::shared_ptr<Request> recv( )
    {   // server 端阻塞取请求
        std::unique_lock<std::mutex> lk( m_mtx );
        m_cv.wait( lk, [this] { return !m_queue.empty( ) || m_stop; } );
        if ( m_stop && m_queue.empty( ) ) return nullptr;
        auto r = m_queue.front( );
        m_queue.pop( );
        return r;
    }
    void stop( )
    {
        {
            std::lock_guard<std::mutex> lk( m_mtx );
            m_stop = true;
        }
        m_cv.notify_all( );
    }

private:
    std::queue<std::shared_ptr<Request>> m_queue;
    std::mutex                           m_mtx;
    std::condition_variable              m_cv;
    bool                                 m_stop = false;
};

// ============ Server: 真实实现 + server stub ============
class CalculatorServer
{
public:
    explicit CalculatorServer( Channel& ch )
        : m_ch( ch )
    {
        m_worker = std::thread( [this] { run( ); } );
    }
    ~CalculatorServer( )
    {
        m_ch.stop( );
        m_worker.join( );
    }

private:
    void run( )
    {
        while ( true )
        {
            auto req = m_ch.recv( );         // 阻塞等待请求
            if ( !req ) break;
            int ret = dispatch( *req );     // 调用真实实现
            req->result.set_value( ret );   // 把结果送回 client(唤醒它)
        }
    }
    int dispatch( const Request& req )
    {    // server stub:解包并路由到真实函数
        if ( req.method == "add" ) return add( req.a, req.b );
        if ( req.method == "mul" ) return mul( req.a, req.b );
        return 0;
    }
    int add( int a, int b ) { return a + b; }   // 真正的业务逻辑
    int mul( int a, int b ) { return a * b; }

    Channel&    m_ch;
    std::thread m_worker;
};

// ============ Client Stub: 把远程调用伪装成本地同步调用 ============
class CalculatorStub
{
public:
    explicit CalculatorStub( Channel& ch )
        : m_ch( ch )
    {
    }

    // 看起来是普通本地函数,实际是一次同步阻塞 RPC
    int add( int a, int b ) { return call( "add", a, b ); }
    int mul( int a, int b ) { return call( "mul", a, b ); }

private:
    int call( const std::string& method, int a, int b )
    {
        auto req = std::make_shared<Request>( );
        req->method = method;                       // 1. 打包参数(marshalling)
        req->a = a;
        req->b = b;
        std::future<int> fut = req->result.get_future( );

        m_ch.send( req );                           // 2. 发送请求

        return fut.get( );                           // 3. 阻塞等待响应 <== 关键阻塞点
    }                                               // 4. 拿到结果作为返回值交回
    Channel& m_ch;
};

int main( )
{
    Channel          channel;
    CalculatorServer server( channel );   // 启动 server 工作线程
    CalculatorStub   stub( channel );     // client stub

    std::cout << "add(3, 4) = " << stub.add( 3, 4 ) << "\n";   // 用起来像本地调用
    std::cout << "mul(6, 7) = " << stub.mul( 6, 7 ) << "\n";
    return 0;
}
