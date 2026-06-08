message(STATUS "Configuring ARM64 toolchain for UP 3588")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(Env3588Path /usr/local/3588Env)

# 设置交叉编译器路径

set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc CACHE FILEPATH "ARM64 C compiler")
set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++ CACHE FILEPATH "ARM64 C++ compiler")

# 设置库查找的前缀和后缀
set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a" ".la")

set(Qt5_DIR "/usr/local/Qt-5.12.0-arm/lib/cmake/Qt5/") 

# 确保使用主机的pkg-config
set(PKG_CONFIG_EXECUTABLE /usr/bin/pkg-config)
# 设置PKG_CONFIG_PATH环境变量，指向ARM平台的pkg-config文件
set(ENV{PKG_CONFIG_PATH} "/usr/local/3588Env/usr/lib/pkgconfig:/usr/local/3588Env/usr/lib/aarch64-linux-gnu/pkgconfig:$ENV{PKG_CONFIG_PATH}")

set(CMAKE_MAKE_PROGRAM /usr/bin/make CACHE FILEPATH "Path to host make")
set(CMAKE_AR /usr/bin/aarch64-linux-gnu-ar CACHE FILEPATH "Path to host ar")
set(CMAKE_SYSROOT ${Env3588Path})
# 设置 sysroot 路径
set(CMAKE_FIND_ROOT_PATH ${Env3588Path}/usr)
set(CMAKE_PREFIX_PATH /usr/local/Qt-5.12.0-arm 
                      ${Env3588Path}/usr/  
                      ${Env3588Path}/usr/lib/aarch64-linux-gnu
                      CACHE STRING "Path to find packages")

# # 或者使用 CMAKE_PREFIX_PATH
set(CMAKE_MODULE_PATH   ${Env3588Path}/usr/lib/pkgconfig 
                        ${Env3588Path}/usr/  
                        /usr/local/Qt-5.12.0-arm/lib/pkgconfig
                        ${Env3588Path}/usr/lib/aarch64-linux-gnu/pkgconfig
                        CACHE STRING "Path to find packages")

set(OPENSSL_ROOT_DIR  ${Env3588Path}/usr)
set(OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT_DIR}/include/aarch64-linux-gnu)
set(OPENSSL_LIB ${OPENSSL_ROOT_DIR}/lib/aarch64-linux-gnu)



# 设置查找模式
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# 添加库路径
# link_directories(${CMAKE_SYSROOT}/usr/)
# link_directories(${CMAKE_SYSROOT}/usr/lib)
link_directories(${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu)
link_directories(${CMAKE_SYSROOT}/usr/)
include_directories(${CMAKE_SYSROOT}/usr/include)
set(CMAKE_LIBRARY_PATH ${Env3588Path}/usr/lib/aarch64-linux-gnu/)      #替换为实际的库路径

#关闭一些编译警告 (注：有些警告不应关闭)
add_compile_options(-Wno-sign-compare)  #有符号和无符号类型比较
add_compile_options(-Wno-unused-function)
add_compile_options(-Wno-unused-parameter)
add_compile_options(-Wno-unused-variable)
add_compile_options(-Wno-unused-result)
add_compile_options(-Wno-unused-but-set-variable)
add_compile_options(-Wno-attributes)
add_compile_options(-Wno-reorder)
add_compile_options(-Wno-unknown-pragmas)
add_compile_options(-Wno-endif-labels)
add_compile_options(-Wno-switch)
add_compile_options(-Wno-comment)
add_compile_options(-Wno-pointer-arith)
add_compile_options(-Wno-misleading-indentation)
add_compile_options(-Wno-parentheses)
# add_compile_options(-Wno-delete-non-virtual-dtor)
add_compile_options(-Wno-return-local-addr)
add_compile_options(-Wall -Wextra )
# -Wall 启用所有常见警告
# -Wextra 启用额外警告 
# -Werror 将警告视为错误
