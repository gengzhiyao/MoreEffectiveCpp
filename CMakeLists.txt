cmake_minimum_required(VERSION 3.10)

project(MoreEffectiveCpp)

add_subdirectory(Item25) # 将 constructor 和 non-member functions 虚化
add_subdirectory(Item26) # 限制某个类所能产生的对象的数量
add_subdirectory(Item27) # 要求或禁止某个类产生于 Heap 中

add_subdirectory(Item29) # 引用计数
add_subdirectory(Item30) # Proxy Class
add_subdirectory(Item31) # 让函数根据一个以上的对象类型来决定如何虚化
