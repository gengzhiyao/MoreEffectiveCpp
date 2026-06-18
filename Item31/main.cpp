#include <iostream>
#include <typeinfo>
#include <map>
#include <variant>

/**
 * @brief 让函数根据一个以上的对象类型来决定如何虚化
 * 1. 使用 RTTI 结合虚函数机制 USE_RTTI_VIRTUAL_FUNCTION
 * 2. 使用虚函数机制 USE_VIRTUAL_FUNCTION_ONLY
 * 3. 自行仿真虚函数表 USE_SIMULATE_VIRTUAL_TABLE
 *    重点在于：如何构建这样一个虚函数表呢？
 *    但该方法仍然具有很大缺点：当加入新的类型时，还是需要在类中添加新的虚函数，违反开闭原则，和先前讨论一致
 */

#pragma region MacroDefinition
// #define USE_RTTI_VIRTUAL_FUNCTION
// #define USE_VIRTUAL_FUNCTION_ONLY
// #define USE_SIMULATE_VIRTUAL_TABLE
// #define USE_NON_MEMBER_FUNCTION
#pragma endregion

#pragma region ProcessCollision
class GameObject;
void ProcessCollision( GameObject& obj, GameObject& obj2 )
{
    // 问题是：如何根据obj和obj2的类型来判断发生碰撞的对象的实际类型是什么？？？
    // 想法是使用 RTTI 结合虚函数机制
}
#pragma endregion

#ifdef USE_VIRTUAL_FUNCTION_ONLY    // 将 double dispatch 以两个 single dispatch 实现
class SpaceShip;
class SpaceStation;
class Asteroid;
#endif

#ifdef USE_SIMULATE_VIRTUAL_TABLE
class SpaceShip;
class SpaceStation;
class Asteroid;
#endif

class GameObject
{
    // ... 内含纯虚函数，作为抽象纯虚基类，不可实例化
#ifdef USE_RTTI_VIRTUAL_FUNCTION
public:
    virtual void collide( GameObject& otherObject ) = 0;
#endif

#ifdef USE_VIRTUAL_FUNCTION_ONLY
public:
    // collide 被重载，每一个版本对应继承体系中的一个
    virtual void collide( GameObject& otherObject ) = 0;
    virtual void collide( SpaceShip& otherObject ) = 0;
    virtual void collide( SpaceStation& otherObject ) = 0;
    virtual void collide( Asteroid& otherObject ) = 0;
#endif

#ifdef USE_SIMULATE_VIRTUAL_TABLE
public:
    virtual void collide( GameObject& otherObject ) = 0;
#endif
};

class SpaceShip /*SpaceStation、Asteroid*/ : public GameObject
{
#ifdef USE_RTTI_VIRTUAL_FUNCTION
public:
    virtual void collide( GameObject& otherObject ) override
    {
        const std::type_info& type = typeid( otherObject );
        if ( type == typeid( SpaceShip ) )
        {
            // process SpaceShip collide with SpaceShip
        }
        else /*if(type == typeid( SpaceStation ))*/
        {// .......其他 else 分支
        }
    }
#endif

#ifdef USE_VIRTUAL_FUNCTION_ONLY
public:
    // collide 被重载，每一个版本对应继承体系中的一个
    virtual void collide( GameObject& otherObject ) override { otherObject.collide( *this ); };
    virtual void collide( SpaceShip& otherObject ) override { /* process SpaceShip collide with SpaceShip */ };
    virtual void collide( SpaceStation& otherObject ) override { /* process SpaceShip collide with SpaceStation */ };
    virtual void collide( Asteroid& otherObject ) override { /* process SpaceShip collide with Asteroid */ };
#endif

#ifdef USE_SIMULATE_VIRTUAL_TABLE
    using HitFunctionPtr = void ( SpaceShip::* )( GameObject& );
    using HitMap = std::map<std::string, HitFunctionPtr>;

    static HitMap initCollisionMap( )   //! 重点：如何构建这样一个模拟的虚函数表呢？
    {
        static HitMap hm;
        hm[typeid( SpaceShip ).name( )] = &hitSpaceShip;
        // hm[ typeid( SpaceStation ).name( ) ] = &hitSpaceStation;
        // hm[ typeid( Asteroid ).name( ) ] = &hitAsteroid;
        return hm;
    }
    // Notion: Optimization：return pointer to HitMap instead of HitMap to avoid copying
    static HitFunctionPtr lookup( const GameObject& whatWeHit )    // 目标：给定一个 GameObject 对象，返回对应的成员函数指针
    {
        // static HitMap collisionMap;     // 用来将一个以字符串呈现的类，对应到 SpaceShip 类的成员函数指针
        static HitMap collisionMap = initCollisionMap( );
        auto          mapEntry = collisionMap.find( typeid( whatWeHit ).name( ) );
        if ( mapEntry == collisionMap.end( ) )
        {
            return nullptr;
        }
        return mapEntry->second;
    }

public:
    virtual void collide( GameObject& otherObject ) override
    {
        HitFunctionPtr hfp = lookup( otherObject );
        if ( hfp )
        {
            ( this->*hfp )( otherObject );  // 调用
        }
        else
        {
            throw std::runtime_error( "No hit function found for unknown object type" ); // 未知 GameObject 类型，抛出异常
        }
    }
    // virtual void hitSpaceShip( SpaceShip& otherObject ); // new virtual function
    virtual void hitSpaceShip( /*SpaceShip*/ GameObject& otherObject ) { SpaceShip& otherShip = dynamic_cast<SpaceShip&>( otherObject ); /* process SpaceShip collide with SpaceShip*/ }
    virtual void hitSpaceStation( SpaceStation& otherObject ); // new virtual function
    virtual void hitAsteroid( Asteroid& otherObject ); // new virtual function
#endif
};

#ifdef USE_NON_MEMBER_FUNCTION
namespace   // 匿名命名空间，仅对本翻译单元可见，类似 static
{
void shipAsteroid( GameObject& ship, GameObject& asteroid ) {}
void shipStation( GameObject& ship, GameObject& spaceStation ) {}
void asteroidStation( GameObject& asteroid, GameObject& spaceStation ) {}

void asteroidShip( GameObject& asteroid, GameObject& ship ) { /*shipAsteroid(ship, asteroid);*/ }   // 对称处理
// ......
using HitFunctionPtr = void ( * )( GameObject&, GameObject& );
using HitMap = std::map<std::pair<std::string, std::string>, HitFunctionPtr>;

std::pair<std::string, std::string> makeStringPair( const char* typeName1, const char* typeName2 ) { return std::make_pair( typeName1, typeName2 ); }

static HitMap initCollisionMap( )
{
    static HitMap hm;
    hm[makeStringPair( "SpaceShip", "Asteroid" )] = &shipAsteroid;
    hm[makeStringPair( "SpaceShip", "SpaceStation" )] = &shipStation;
    hm[makeStringPair( "Asteroid", "SpaceStation" )] = &asteroidStation;
    return hm;
}

HitFunctionPtr lookup( const std::string& className, const std::string& otherClassName )
{
    static HitMap collisionMap = initCollisionMap( );
    auto          mapEntry = collisionMap.find( std::make_pair( typeid( className ).name( ), typeid( otherClassName ).name( ) ) );
    if ( mapEntry == collisionMap.end( ) )
    {
        return nullptr;
    }
    return mapEntry->second;
}

void ProcessCollision( GameObject& ship, GameObject& otherObject )
{
    HitFunctionPtr hfp = lookup( typeid( ship ).name( ), typeid( otherObject ).name( ) );
    if ( hfp )
    {
        hfp( ship, otherObject );
    }
    else
    {
        throw std::runtime_error( "No hit function found for unknown object type" ); // 未知 GameObject 类型，抛出异常
    }
}

class CollisionMap
{
public:
    using HitFunctionPtr = void ( * )( GameObject&, GameObject& );
    void                 addEntry( const std::string& className, const std::string& otherClassName, HitFunctionPtr hfp ) { /*add entry*/ }
    void                 removeEntry( const std::string& className, const std::string& otherClassName );
    HitFunctionPtr       lookup( const std::string& className, const std::string& otherClassName );
    static CollisionMap& thecollisionMap( )
    {
        static CollisionMap cm;
        return cm;
    }

private:
    CollisionMap( ) {}
    CollisionMap( const CollisionMap& ) = delete;
};
// Example:
void shipSun( GameObject& ship, GameObject& sun ) {}
}    // namespace
#endif

#pragma region ModernMultiDispatch
struct Rocket
{
};
struct AirCraft
{
};
struct Satellite
{
};

using GameObj = std::variant<Rocket, AirCraft, Satellite>;

void collide( const GameObj& a, const GameObj& b )
{
    std::visit(
        []( auto&& x, auto&& y )
        {
            using X = std::decay_t<decltype( x )>;
            using Y = std::decay_t<decltype( y )>;
            if constexpr ( std::is_same_v<X, Rocket> && std::is_same_v<Y, AirCraft> )
                std::cout << "rocket vs airCraft: boom\n";
            else if constexpr ( std::is_same_v<X, AirCraft> && std::is_same_v<Y, Satellite> )
                std::cout << "airCraft vs satellite: damaged\n";
            else
                std::cout << "generic collision\n";
        },
        a,
        b );
}
#pragma endregion

int main( )
{
#ifdef USE_NON_MEMBER_FUNCTION
    CollisionMap::thecollisionMap( ).addEntry( "SpaceShip", "Sun", &shipSun );
#endif

    collide( Rocket( ), AirCraft( ) );    // C++17 modern multi-dispatch
    return 0;
}