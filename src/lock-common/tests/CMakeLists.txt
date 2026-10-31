add_executable(LockStyleTest lock_style_test.cc)
target_link_libraries(LockStyleTest PRIVATE
    gxdm-lock-common
    Qt6::Test
)
add_test(NAME LockStyle COMMAND LockStyleTest)

add_executable(LockBackendTest lock_backend_test.cc)
target_link_libraries(LockBackendTest PRIVATE
    gxdm-lock-common
    Qt6::Test
)
add_test(NAME LockBackend COMMAND LockBackendTest)
