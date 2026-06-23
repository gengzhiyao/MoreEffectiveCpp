#include <iostream>

/**
 * ! 智能指针
 * 智能指针由 *模板* 产生出来，由于其像内建的指针一样，所以它必须有强烈的类型相关性，并且需要作者指定其是否应该存在拷贝、赋值的行为
 * 智能指针的基本实现如 region SimpleImplementationOfSmartPointers
 * 其使用全貌如 Appetizers ，意即：智能指针的使用应该和裸指针一样，其实现了 operator-> 和 operator*
 * 1. 智能指针的构造、赋值和析构
 * 作者需要控制智能指针是否需要拷贝以及赋值，并控制拷贝和赋值时，程序执行的逻辑；对于构造来说，可以要求构造时传参，也可以在使用时判断
 * 2. 智能指针的解引用与成员访问
 * 解引用操作符返回值必须是引用，如果返回值，那么会：(1) 多态失效 (2) 对象切片 (3) 生成新对象，低效和 UB
 * 成员访问操作符一般情况下直接返回对应的裸指针形式 T*
 * 3. 测试智能指针是否为 nullptr
 * (1) 智能指针类内提供 operator void* ：会使风马牛不相及的两个指针判等操作编译通过，运行期一直返回 false
 * (2) 提供 operator! 无用 鸡肋，在比较时仍然报错
 * 4. 提供返回裸指针的方法
 * (1) 直接在智能指针类的内部提供一个隐式类型转换操作符重载 operator T*()，直接解决了测试智能指针是否为 nullptr 的问题，但隐式类型转换非常隐蔽，其提供了对外获取指针 Handle 的接口，可以被保存并 delete
 * (2) 向标准库学习，提供接口获取裸指针
 * 5. 在继承体系下，智能指针的类型转换
 * (1) 由于智能指针采用模板实现，所以不同的模板参数实例化出来的智能指针类型不一致，智能指针之间并不存在继承关系，因此不存在如裸指针一样的隐式类型转换
 * (2) 不切实际的方法：实例化继承体系中的类，并在其中添加向基类智能指针的隐式转换函数
 *      不仅违反开闭原则，而且在继承体系中，需要在继承谱中为每个类都添加隐式转换函数，而且是*直接的*隐式转换，因为简介的隐式转换编译器需要做两次转换，但在编译时，只能有一次用户自定义的隐式转换
 * (3)
 * 利用成员函数模板，在类模板中添加成员函数模板，在模板匹配时，自动推导成员函数模板的模板参数，该方法利用到了裸指针在继承体系中的隐式转换。该方法虽好，但是在实例化期间，继承体系复杂时，可能存在多个合法的模板匹配，造成二义性
 * 6. 智能指针与 const 不期而遇
 * 智能指针与 const 之间的问题犹如继承体系，同样是由于类型不一致造成的，但这对于裸指针来说，却是非常常见的。具体见下吧。
 */

#define USE_INHERITANCE
// #define USE_MODERN_SFINAE

#pragma region SimpleImplementationOfSmartPointers
template <typename T>
class SmartPtr
{
public:
    SmartPtr( T* real )
        : pointee( real )
    {
    }
    SmartPtr( const SmartPtr& rhs ) = delete;   // 是否允许拷贝？
    SmartPtr& operator=( const SmartPtr& rhs ) = delete;    // 是否允许赋值？
    ~SmartPtr( ) { delete pointee; }
    T* operator->( ) const;
    T& operator*( ) const;  // 解引用

#ifdef USE_INHERITANCE
    template <class newType>                 // 成员模板
    operator SmartPtr<newType>( )            // 生成到任意 SmartPtr<newType> 的转换
    {
        // newType 不是被"推导"出来的，而是由编译器在寻找类型转换时，根据它当前需要的目标类型反过来"代入"确定的。
        return SmartPtr<newType>( pointee );  // 用 pointee 构造目标智能指针
    }
#endif

#ifdef USE_MODERN_SFINAE
// 现代写法：用 enable_if 把“T* 必须能转 newType*”这个条件提到签名里
    template <class newType, class = std::enable_if_t<std::is_convertible_v<T*, newType*>>>
    operator SmartPtr<newType>( ) const;

// 或 C++20 concepts / requires
    template <class newType>
        requires std::convertible_to<T*, newType*>
    operator SmartPtr<newType>( ) const;
#endif
private:
    T* pointee; // 实际所指之物
};
#pragma endregion

#pragma region Appetizers // 开胃菜
// 本段作用：目的是让你直观看到——智能指针在使用语法上和裸指针一模一样，但背后可以偷偷干很多裸指针干不了的事
template <typename T>
class DBPtr
{
};
class Tuple
{
    // 作用：绑定数据库里的一条记录
};

// DBPtr<Tuple>，表示"指向某条数据库记录的智能指针"。 Raw: Tuple* ptr = new Tuple;

// logEntry（记日志）这部分，是用来回答"那智能指针到底比裸指针强在哪"的——它展示了裸指针做不到、而智能指针能透明完成的"幕后工作"。
// 你没法让一个 Tuple* 在每次被解引用时自动写日志，但智能指针可以，因为它能重载 operator->、operator*，在这些操作里塞进任意逻辑。而这一切对客户完全透明——语法还是 pt->method()。
template <typename T>
class LogEntry
{
    // 在分布式数据库这个场景下，幕后可以发生的事情包括:
    // 1. 远程/本地透明化：这个 Tuple 可能在本机内存里，也可能在远程机器上。当你 pt->... 解引用时，智能指针的 operator->
    // 可以先判断对象在哪，远程的话就通过网络把它取过来——但客户代码完全不用关心这件事。
    // 2. 访问日志（这就是 logEntry 干的事）：每次通过 pt 访问这条记录，智能指针都可以顺手往日志里记一笔："谁、什么时候、访问了哪条 tuple"。做审计、调试、计费都用得上。
public:
    LogEntry( const T& objToBeModified );
};

#pragma endregion

// Notion: auto_ptr 相关的知识过时... 现在完全采用 unique_ptr

#pragma region Dereference && operator->

// Implement Dereferencing Operator
// 实现解引用操作符的返回值必须是 T& 而非 T
// 返回 T 会造成对象的生成，多态的失效，对象切片，效率降低，UB

// operator-> 可以返回 raw pointer 或者 smart pointer

#pragma endregion

#pragma region IsSmartPointBeNull
// 测试智能指针是否为 null
class TreeNode
{
};
SmartPtr<TreeNode> ptn( new TreeNode );
// ptn == nullptr ??? 如果 SmartPtr 中未实现 operator== ，那么编译不通过
// 书中给出办法，实现 operator void*(); 但是在涉及 SmartPtr<Apple> obj1 == SmartPtr<Orange> obj2 时，反而能编译通过! 这可是两种不同的对象啊
// 并且 addressof(obj1) != addressof(obj2)，判断条件永远不成立
// 若实现 operator! 那么 if(!obj) 在 obj = null 时成立，但 obj1 == obj2 仍然编译报错
#pragma endregion

#pragma region TransformToRawPointer

// 智能指针中实现 operator T*() 那么测试指针是否为 null 的问题一并解决了
// 但这也向外暴露了指针，一旦有人保留了 Handle ，那么就违反了 Item5
// 本段的意图是：不要通过隐式转换 operator T*() 得到裸指针，标准库中采用的是显示调用函数的方式得到裸指针

#pragma endregion

#pragma region InheritanceWithSmartPointer

class MusicProduct
{
};
class Cassette : public MusicProduct
{
};   // 磁带
class CD : public MusicProduct
{
};

void displayAndPlay( const MusicProduct* pmp, int howMany ) { std::cout << "use raw" << std::endl; };  // 裸指针可以自由地"派生类指针 → 基类指针"隐式转换
void displayAndPlay( const SmartPtr<MusicProduct>& pmp, int howMany )
{
    std::cout << "use smart" << std::endl;
};    // SmartPtr<Cassette> 和 SmartPtr<MusicProduct> 是两个毫不相干的类型，继承关系不会自动"传染"到智能指针上

// 笨办法：为每个目标类型写一个转换操作符
// operator SmartPtr<MusicProduct>()
// 但这完全不可行 它只解决了"转到 MusicProduct"这一个目标。如果体系里还有更上层的基类、或者要转 const 版本，你得为每一种目标类型手写一个转换操作符 完全违反开闭原则
// 正解：member template（成员模板）生成转换函数

#pragma endregion

#pragma region SmartPointer && Const

// const CD* = CD* 这种赋值方式可以，但智能指针不行，因为类型不一致
// SmartPtr<const CD> SmartPtr<CD> 不是一个类型，赋值会编译报错

template <typename T>
class SmartPtrToConst
{
public:
    //... functions...
protected:
    union
    {
        const T* constPointee;
        T*       pointee;
    };
};

template <typename T>
class SmartPointer : public SmartPtrToConst<T>
{
public:
};

#pragma endregion

int main( )
{
#ifdef USE_INHERITANCE
    SmartPtr<Cassette> funMusic( new Cassette );
    displayAndPlay( funMusic, 10 );
    // 推导链：把实参 funMusic（类型 SmartPtr<Cassette>）绑定到形参 pmp（类型 const SmartPtr<MusicProduct>&），类型不匹配，于是去找一个能从 SmartPtr<Cassette> 转到 SmartPtr<MusicProduct> 的转换。
    // 编译器检查 SmartPtr<Cassette> 的成员，发现有个转换操作符模板 operator SmartPtr<newType>()。现在它要回答："newType 取什么，才能让这个转换的结果类型正好是我需要的 SmartPtr<MusicProduct>
    // 把目标类型 SmartPtr<MusicProduct> 和操作符的返回模式 SmartPtr<newType> 对齐——一眼就能匹配出 newType = MusicProduct。
    // 于是实例化出这个具体函数: operator SmartPtr<MusicProduct>() { return SmartPtr<MusicProduct>(pointee); }   // pointee 是 Cassette*
    // 实例化能不能成功，取决于函数体 SmartPtr<MusicProduct>(pointee) 合不合法——也就是 Cassette* 能不能构造 SmartPtr<MusicProduct>，最终归结到 Cassette* → MusicProduct* 能不能转。能，于是整条通过。

    // Q: 是 SFINAE 吗？
    //! 核心机制不是 SFINAE。这里发生的是一次普通的"模板实例化失败 → 硬错误（hard error）"，而不是 SFINAE 的"悄悄剔除候选 → 当作没看见"
    // 两者的区别正好卡在一个关键点上：失败发生在哪一步。
    //! SFINAE的成立有个严格前提：替换失败必须发生在**函数签名（immediate
    //! context，即直接上下文）**里——模板参数、返回类型、形参类型这些地方。只有这个区域的替换失败，才会被"原谅"，把候选函数从重载集里默默剔除。
    // 这里的失败发生在函数体里，不受 SFINAE 保护——它是一个编译错误，编译器不会"当没看见"然后去找别的候选，而是直接报错停下。所以这是：签名替换成功 → 候选被选中 → 实例化函数体 → 体内失败 → hard
    // error。这条路径和 SFINAE 没关系。 现代写法常常主动把这个约束提到签名里，让它真正变成 SFINAE / 概念约束，从而既挡住非法转换、又给出干净的错误，还能正确参与重载决议
#endif
    return 0;
}