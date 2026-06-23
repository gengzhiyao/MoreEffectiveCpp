#include <iostream>

/**
 * @brief 限制某个类所能产生的对象的数量
 * 1. 允许 0 个或 1 个对象
 * 将构造函数私有化，通过单例模式，只允许一个对象存在；有两种方式实现获取单例模式的对象：
 * 静态成员函数、友元函数(又分为全局函数或命名空间中的普通函数)、静态成员变量
 * ! 静态成员变量作为获取单例模式的入口，及其不推荐使用，为什么呢？
 * ^首先是静态成员变量的初始化顺序问题：静态成员变量和全局变量的初始化顺序在跨编译单元时是不确定的，链接器只是简单拼接所有 .o 的数据段；
 * 程序启动时，按照拼接顺序挨个初始化。静态成员单例在编译时，它的内存地址就已经确定。链接完，地址就固定不变，运行时只是调用构造函数。
 * 如果在 B.cpp 中使用单例，在 A.cpp 中定义单例，按照字母顺序拼接的话，那么 B.cpp 中的单例对象的地址是不确定的，使用时会导致未定义行为。
 * ^其次是内存空间问题：静态成员变量不管你有没有使用，都会生成一个对象；但是静态局部变量只是在第一次使用时才会初始化，不用就不会初始化，不占用内存空间。
 * 另有一种实现方式是引用计数：即在类中添加一个引用计数成员变量，每次获取单例对象时，引用计数加 1；每次销毁单例对象时，引用计数减 1。该方式可以泛化为任意数量的对象。
 * 但是这种方式却存在一定的问题：
 * 在继承体系中，如果构造函数是 public 的，那么子类可以继承父类的引用计数成员变量。也就是说，子类在实例化时，父类那部分的引用计数也会增加 1。
 * 这就会导致诸如彩色打印机 ColorPrinter:Printer(继承)/ColorPrinter::Printer(内含) 实例化之后，其无法实例化的问题。
 * 问题就出在 Printer 对象可以在 3 种不同的状态下生存：1. 本身 2. 派生类中的基类成分 3. 作为成员内嵌于对象中。
 * 修改方式很简单，将构造函数私有化即可。直接阻止派生。
 * * 将构造函数私有化，不一定非要和限制有限个对象联想在一起。构造函数私有化还可以用于实现两个独立需求。1. 阻止派生 2. 允许外部创建任意多个对象，有限状态机 FSA
 * 2. 虽然通过单例限制了一个对象的存在，但是在不同的时间中，我们如果销毁上一个对象，而产生下一个对象，理论是每一时刻都只有一个对象存在；
 * 但是由于局部静态对象是栈区对象，我们无法销毁一个局部静态对象，也就是所，我们不可能针对栈区对象实施 delete ！
 * 限制能够产生的对象的数量，便可以使用引用计数的方式。配合有限状态机 FSA，我们可以实现任意数量的对象。
 * 3. 实现一个用于计算对象个数的 Base Class
 */

#pragma region Printer
#define USING_NAMESPACE_PRINTER

#ifdef USING_NAMESPACE_PRINTER
namespace PrinterNamespace
{
#endif

class Printer
{
public:
    static Printer& thePrinter( )   // 静态成员函数获取单例对象
    {
        static Printer instance;
        return instance;
    }
    friend Printer& thePrinter( );   // 友元函数获取单例对象
    // static Printer thePrinter; 静态成员变量获取单例对象

private:
    Printer( ) = default;   // private 构造函数
};

Printer& thePrinter( )   // 友元函数实现码 注意并未使用 inline! 为什么？因为 inline 函数会在调用处展开，目标码可能在不同的编译单元中展开多次，从而可能产生多个单例对象！
{
    // ! 所以，不要将内含静态成员变量的函数定义为 inline 函数！ 不过，这个问题在 C++17 中已经明确解决了。
    static Printer instance;
    return instance;
}
#ifdef USING_NAMESPACE_PRINTER
}
#endif
#pragma endregion

#pragma region Finite State Automaton
#define USING_FSA  // 有限状态机
#ifdef USING_FSA
class FSA
{
public:
    static FSA* makeFSA( ) { return new FSA( ); }   // 伪构造函数：返回的指针指向独立的对象
    static FSA* makeFSA( const FSA& rhs ) { return new FSA( rhs ); }

private:
    FSA( )
    {
        if ( ++m_ObjectCount > m_MaxObjectCount ) throw std::runtime_error( "Object count exceeds max count" );
        std::cout << "FSA constructor" << std::endl;
    }
    FSA( const FSA& );
    static int           m_ObjectCount;   // 当前对象数量
    static constexpr int m_MaxObjectCount = 3;   // 限制最大对象数量
};
#endif

int FSA::m_ObjectCount = 0;

#pragma endregion

#pragma region Ref Count Base Class
template <typename BeingCounted>
class RefCountBase  // 用于计算对象个数的 Base Class
{
public:
    static int GetObjectCount( ) { return m_ObjectCount; }  // 获取当前对象数量

protected:
    RefCountBase( ) { Init( ); }
    RefCountBase( const RefCountBase& rhs ) { Init( ); }
    ~RefCountBase( ) { --m_ObjectCount; }

private:
    static int       m_ObjectCount;   // 当前对象数量
    static const int m_MaxObjectCount;   // 限制最大对象数量

    void Init( )
    {
        if ( ++m_ObjectCount > m_MaxObjectCount ) throw std::runtime_error( "Object count exceeds max count" );
    }
};

class CountedPrinter : private RefCountBase<CountedPrinter> // 继承 RefCountBase 类
{
public:
    static CountedPrinter* MakePrinter( ) { return new CountedPrinter( ); }

    using RefCountBase<CountedPrinter>::GetObjectCount; // 由于是私有继承，使用 using 恢复其 public 访问权限

private:
    CountedPrinter( ) = default;    // 已经有 RefCountBase 为其计算对象个数，因此该构造函数中就可以执行自己的初始化逻辑
    CountedPrinter( const CountedPrinter& rhs ) = default;
};

template <typename BeingCounted>
int RefCountBase<BeingCounted>::m_ObjectCount = 0;  // 静态成员变量的初始化放在外部

// 针对 m_MaxObjectCount，无为而治，由用户负责指定。例如用户在某个实现文件中加入：
template <> // 类模板的静态常量成员，针对某个实例化类型做显式特化赋值时 加 template<> 代表完全显式特化
const int RefCountBase<CountedPrinter>::m_MaxObjectCount = 3;

#pragma endregion

int main( )
{
    for ( size_t i = 0; i < 4; i++ )
    {
        FSA::makeFSA( );
    }   // Exception: what():  Object count exceeds max count

    return 0;
}