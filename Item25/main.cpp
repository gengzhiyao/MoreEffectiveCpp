#include <iostream>
#include <fstream>
#include <list>

/**!
 * @brief 将 constructor 和 non-member functions 虚化
 * ! 正常情况下，构造函数和非成员函数不能够虚化；这里所说的虚化是概念上的虚化，而不是语法上的虚化
 * 1. 虚构造函数：观念上指的是工厂方法模式：即能够根据不同的参数创建不同的对象，他的行为像构造函数一样能够产生对象；又因为能够根据参数产生不同的对象，所以他的行为像虚函数。
 *  代码中典型的就是 NewsLetter 的 CreateComponent。通过读取磁盘中的文件，读取对象信息并生成对应的对象。
 * 2. 虚拷贝构造函数：解决多态拷贝问题。 clone() 函数
 *  Notion: 当你只有基类的指针或引用时，无法正确拷贝出完整子类对象 Shape* p = new Circle(); // 基类指针，指向子类对象  Shape* p2 = new Shape(*p); //
 *  基类拷贝构造函数，只能拷贝出基类部分，子类部分会被丢失，对象切片。 利用协变：虚函数 NLComponents::clone() 返回值可以是子类指针，该虚函数内部什么也不做，只是调用子类对象真正的拷贝构造函数。
 *  可见：当真正的拷贝构造函数浅拷贝时，clone() 函数执行浅拷贝，当真正的拷贝构造函数深拷贝时，clone() 函数执行深拷贝。 所以这里的 copy/clone 是和 copy constructor 意思一样。
 * 3. 非成员函数的虚化：operator<< 和 print 函数
 *  例：本节中 全局的 operator<< 函数
 *  比如：我们想让 TextBlock 和 Graphics 具备输出功能，以便能够在 NewsLetter 中打印。最直接的想法是，自定义 operator<<
 *  如 NLComponents::operator<<(std::ostream& os)，但是在使用时就是非常怪异。因为我们会将 std::ostream 作为参数传递进去，这时候的语法成了：graphics << std::cout << std::endl;
 *  而正常情况下，应该是：std::cout << graphics << std::endl; 那么如何实现这种功能呢？
 *  实现一个全局的 operator<< 函数，签名为：std::ostream& operator<<( std::ostream& os, const NLComponents& component ); 在使用时，编译器会根据 ADL 查找规则，查找并匹配合适的 operator<< 函数。
 *  非成员函数，内部调用虚函数，让其看起来像虚成员函数。
 */

class NLComponents
{
public:
    virtual NLComponents* clone( ) const = 0;   // 协变返回类型：virtual copy constructor
    virtual std::ostream& operator<<( std::ostream& os ) const = 0;
    virtual std::ostream& print( std::ostream& os ) const = 0;
    virtual void          CustomPrintThisObject( ) const = 0;   // 自定义工具函数，用于打印对象的详细信息，与本节无关
    virtual ~NLComponents( ) = default;
};

class TextBlock : public NLComponents
{
public:
    TextBlock*            clone( ) const override { return new TextBlock( *this ); }
    virtual std::ostream& operator<<( std::ostream& os ) const override { return os << "TextBlock operator<<"; }
    virtual std::ostream& print( std::ostream& os ) const override { return os << "TextBlock print"; }
    virtual void          CustomPrintThisObject( ) const override { std::cout << "This is TextBlock CustomPrintThisObject" << std::endl; }
};

class Graphics : public NLComponents
{
public:
    Graphics*             clone( ) const override { return new Graphics( *this ); }
    virtual std::ostream& operator<<( std::ostream& os ) const override { return os << "Graphics operator<<"; }
    virtual std::ostream& print( std::ostream& os ) const override { return os << "Graphics print"; }
    virtual void          CustomPrintThisObject( ) const override { std::cout << "This is Graphics CustomPrintThisObject" << std::endl; }
};

class NewsLetter
{
public:
    explicit NewsLetter( std::ifstream& file )
    {
        if ( !file )
        {
            throw std::runtime_error( "文件未打开或读取失败" );
        }

        std::string line;
        while ( std::getline( file, line ) )
        {
            components.push_back( CreateComponent( line ) );
        }

        for ( const auto& component : components )
        {
            component->CustomPrintThisObject( );
        }
    }

    NewsLetter( const NewsLetter& other )
    {
        for ( auto component : other.components )
        {
            components.push_back( component->clone( ) );    // 这里只有基类指针，却需要拷贝子类对象，因此利用 virtual copy constructor
        }
    }

    static NLComponents* CreateComponent( const std::string& type )
    {
        if ( type == "TextBlock" )
        {
            return new TextBlock( );
        }
        else if ( type == "Graphics" )
        {
            return new Graphics( );
        }
        else
            return nullptr;
    }

private:
    std::list<NLComponents*> components;
};

inline std::ostream& operator<<( std::ostream& os, const NLComponents& component ) { return component.print( os ); }

int main( )
{
    std::ifstream file( "../Item25/Object.txt" );
    NewsLetter    news_letter( file );

    TextBlock text_block;
    Graphics  graphics;

    graphics << std::cout << std::endl;    // graphics.operator<<(std::cout);   语法非常怪异

    std::cout << text_block << std::endl;   // operator<<( std::cout, text_block );

    return 0;
}