
#将当前目录下所有的源码文件放入SRC_LIST列表中
aux_source_directory(. SRC_LIST)

#将db目录下的源码文件放入DB_LIST列表中
aux_source_directory(./db DB_LIST)

#将model目录下的源码文件放入MODEL_LIST列表中
aux_source_directory(./model MODEL_LIST)

#将redis目录下的源码文件放入REDIS_LIST列表中
aux_source_directory(./redis REDIS_LIST)

#指定生成的可执行文件的文件名，以及生成这个可执行文件所需要的源文件
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

#指定可执行文件链接时所需要的库文件
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)
