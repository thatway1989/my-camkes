# my-camkes
Implementation of AUTOSAR based on SEL4 microkernel Camkes .

一、准备工作

此git库建立在熟悉sel4，并且练习完https://docs.sel4.systems/Tutorials/的基础上。

代码下载编译，参考资料：https://docs.sel4.systems/projects/camkes-arm-vm/

repo init -u https://github.com/seL4/camkes-vm-examples-manifest

repo sync

mkdir build

cd build

../init-build.sh -DCAMKES_VM_APP=vm_minimal -DPLATFORM=qemu-arm-virt

ninja

./simulate

运行起来后，会进入虚拟机的linux，说明代码没有问题，在此基础上进行二次开发。

二、下载此git库到代码目录

此git库可以实现linux虚拟机和组件之间通信。

cd projects

git clone https://github.com/thatway1989/my-camkes.git

查看init-build.sh代码，发现会在根目录找CMakeLists.txt文件，找不到就按easy-settings.cmake去执行了。

查看easy-settings.cmake文件链接到了vm-examples。修改链接到projects/my-camkes/easy-settings.cmake。

cd ..

rm easy-settings.cmake

ln -s projects/my-camkes/easy-settings.cmake easy-settings.cmake

ln -s projects/my-camkes/myOS/ myOS

这样myOS在代码根目录，里面是camkes构架的。已有组件可以实现linux虚拟机和组件之间通信。

三、linux命令行触发sel4组件执行

linux已经集成了读写发送程序，参考：projects/my-camkes/myOS/overlay_files/init_scripts/cross_vm_test

dataport_read /dev/uio0 4096    #读共享内存

echo -ne "XXXX\n\0" | dataport_write /dev/uio0 4096   #写入共享内存

emits_event_emit /dev/uio0    #发送激活事件

发送激活事件后，projects/my-camkes/myOS/components/CrossvmInit/src/crossvm_init.c

里面的run函数启动后会一直ready_wait()等待，直到linux终端执行了emits_event_emit命令，就继续往下执行。

linux写入的共享内存可以在(char *)dest这个变量里面读取到。
