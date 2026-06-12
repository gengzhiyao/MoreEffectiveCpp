#include <iostream>
#include <list>
#include <algorithm>
/**
 * @brief 要求或禁止某个类产生于 Heap 中
 * 1. 要求对象只能在 Heap 中产生
 * Notion: 将析构函数设为 private，本质是禁止编译器在任何地方，帮助我们隐式调用析构函数，所有的析构动作，只有程序员自己手动完成。譬如说，实现一个 public 伪析构函数，内部调用 private 的析构函数。
 * 自动插入析构函数的情况：1. 函数作用域的栈对象 2. 类作用域中的值成员 3. 继承关系下，子类析构函数中会调用基类的析构函数 DerivedObj->Base::~Base();
 * 在继承关系中，将基类的析构函数设为 private，那么派生类就无法通过编译，这时候需要将父类的析构函数设为 protected 形式。在 has-a 关系中，类中只能有对象的指针成员，不能有对象的值成员。
 * 2. 判断对象是否位于 Heap 中
 * 一旦将基类的析构函数设置为 protected，并且派生类并没有设置析构函数为 private/protected，那么派生类就可以在Stack中产生。这意味着：派生类的基类部分，仍然是位于 Stack 中的。
 * !以上方案只能约束「完整的 UPNumber 对象」不能在栈，但约束不住派生类内嵌的基类子对象。
 * 如果硬性要求：任何形态的 UPNumber 实体（完整独立对象、派生内的基类子对象），内存都必须在堆，如何设计？
 * (1) 重载 operator new 做标记可以吗？
 * 当对象被分配于 Heap 中，operator new 和 constructor 会被调用。可以在其中做标记；另外，还会涉及到表达式求值顺序的问题，编译器可能产出顺序不一致的代码。
 * (2) 不具移植性的方案
 * 深层次利用 Stack 和 Heap 在进程空间中地址的分布情况。直接比较地址。但这种方案不现实哈。
 * ! ***很遗憾，没有办法能够区分堆对象和栈对象。***
 * 回归问题的本质：在工程上，我们为什么想要判断对象是否位于 Heap 中？ [ 无非是为了判断是否能安全的针对对象使用 delete ]
 * 那么解决方案其实还是要针对 operator new。因为决定对象是否在 Heap 中，就是 operator new 被调用。
 * 普通的 operator new 无法考虑到多重继承的问题，因为在多重继承体系中，子类对象的地址是不确定的，涉及到一定的偏移量。
 * 作者给出了 HeapTracked 类的实现。但是，对于解决数组问题的 operator new[]，其实现仍然仍然较难以满足。
 * 3. 禁止对象在 Heap 中产生
 * 将 operator new/new[] 私有化。但是要注意的一点是：在继承体系中，如果子类中声明了了 operator new/new[] 就会隐藏父类中的 operator new/new[]。
 * 这时候，子类仍然可以在 Heap 中产生。
 */

// #define USE_PROTECTED_DESTRUCTOR

class UPNumber
{
public:
    UPNumber( ) = default;
    void Destroy( ) { delete this; }    // 伪析构函数

private:    //! 私有析构函数
#ifdef USE_PROTECTED_DESTRUCTOR
protected:
#endif
    ~UPNumber( ) = default; // 在程序编译好的二进制文件中，离开作用域的那段代码中，编译器向那里隐式插入了析构函数的代码
};

#ifdef USE_PROTECTED_DESTRUCTOR

class NonNegativeUPNumber : public UPNumber
{
public:
    NonNegativeUPNumber( ) =
        default;   //! [ERROR] :use of deleted function 'NonNegativeUPNumber::NonNegativeUPNumber()' => C++ 中的规则：如果隐式析构函数格式非法 → 整个类的隐式默认构造函数被隐式删除。
};

#endif

/** @brief 伪代码***/
// void* Operator::operator new( std::size_t size )
// {
//     onHeap = true;
//     return ::operator new( size );
// }
// 
// Operator::Operator( )
// {
//     if not onHeap
//         throw an exception;
//     proceed......
//     onHeap = false;
// }
// 做好标记，构造时首先检查标记，构造完成后，重置标记。
// 但以上代码在使用 operator new[] 时失效！或许你可能会想，是否可以重载 operator new[] 以模仿 operator new 的行为？
// 答案是：不可以，在 operator new[] 只会被调用一次以分配连续的一大块内存。假设有多个对象被分配于 Heap 中
// 第一次构造连续的对象时，会调用 operator new[] 时，onHeap 会被 true 标记，然后构造函数被调用，初始化后恢复标记。
// 但当构造第二个对象时，只调用 constructor，不会调用 operator new[]，明明是在 Heap 中恢复的，但还是会直接抛出异常。

class HeapTracked
{
public:
    virtual ~HeapTracked( ) = 0;
    static void* operator new( std::size_t size );
    static void  operator delete( void* p );
    bool         IsHeap( ) const;

private:
    using RawAddress = const void*;
    static std::list<RawAddress> heapAddresses;
};

std::list<HeapTracked::RawAddress> HeapTracked::heapAddresses;  // 静态成员定义

HeapTracked::~HeapTracked( ) {}

void* HeapTracked::operator new( std::size_t size )
{
    void* p = ::operator new( size );   // operator new 返回申请到的内存的起始地址
    heapAddresses.push_back( p );
    return p;
}

void HeapTracked::operator delete( void* p )
{
    auto it = std::find( heapAddresses.begin( ), heapAddresses.end( ), p );
    if ( it != heapAddresses.end( ) )
    {
        heapAddresses.erase( it );
        ::operator delete( p );
    }
    else
        throw std::runtime_error( "HeapTracked::operator delete: p not found in heapAddresses" );
}

bool HeapTracked::IsHeap( ) const
{
    const void* rawAddr = dynamic_cast<const void*>( this );    // dynamic_cast 的特殊能力：返回指向最基类的指针
    // 尤其是在多重继承体系中，同一个完整对象里，不同基类指针 Base1 和 Base2 所拿到的子对象地址不一样，都不等于完整Derived对象最开头内存地址
    auto it = std::find( heapAddresses.begin( ), heapAddresses.end( ), rawAddr );
    return it != heapAddresses.end( );
}

class Asset : public HeapTracked
{
public:
    Asset( int initValue ) {}
    ~Asset( ) = default;

private:
    // UPNumber upn;
};

void inventoryAsset( const Asset* asset )
{
    if ( asset->IsHeap( ) )
    {
        std::cout << "Asset is in heap" << std::endl;
    }
    else
    {
        std::cout << "Asset is in stack" << std::endl;
    }
}

int main( )
{
    // UPNumber num;   //! [ERROR] : 离开作用域时，编译器会插入调用析构函数的代码段
    UPNumber* upn = new UPNumber( );
    // delete upn;  //! [ERROR] : 手动调用析构函数，同样会使编译器插入调用析构函数的代码段
    upn->Destroy( );
#ifdef USE_PROTECTED_DESTRUCTOR
    NonNegativeUPNumber upnNonNegativeUP;   //* [WARN] : NonNegativeUPNumber 中的基类成分 UPNumber 是位于 Stack 中的！
#endif

    Asset asset( 100 );
    inventoryAsset( &asset );
    Asset* asset2 = new Asset( 200 );
    inventoryAsset( asset2 );
    return 0;
}