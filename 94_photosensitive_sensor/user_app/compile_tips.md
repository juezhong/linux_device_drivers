# 1. 确认真实的 libatomic.so 存在
ls -l /usr/arm-linux-gnueabihf/lib/libatomic.so

# 2. 创建 libatomic_asneeded.so 软链接
sudo ln -s /usr/arm-linux-gnueabihf/lib/libatomic.so /usr/arm-linux-gnueabihf/lib/libatomic_asneeded.so

# 3. 确认软链接是否创建成功
ls -l /usr/arm-linux-gnueabihf/lib/libatomic_asneeded.so

# 4. 重新编译
arm-linux-gnueabihf-gcc app.c -o app

# 5. 确认生成的是 ARM 可执行文件
file app


4. 删除这个临时补丁

只会删除软链接，不会删除真正的库：

sudo rm /usr/arm-linux-gnueabihf/lib/libatomic_asneeded.so
