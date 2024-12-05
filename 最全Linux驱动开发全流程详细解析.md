

# 最全Linux驱动开发全流程详细解析
————————————————————————————————

## 一、驱动概念

**驱动与底层硬件直接打交道，充当了硬件与应用软件中间的桥梁。**

- 具体任务
  1. 读写设备寄存器（实现控制的方式）
  2. 完成设备的轮询、中断处理、DMA通信（CPU与外设通信的方式）
  3. 进行物理内存向虚拟内存的映射（在开启硬件MMU的情况下）
- 说明：设备驱动的两个任务方向
  1. 操作硬件（向下）
  2. 将驱动程序通入内核，实现面向操作系统内核的接口内容，接口由操作系统实现（向上）
     （**驱动程序**按照操作系统给出的**独立于设备的接口设计**，**应用程序**使用操作系统**统一的系统调用接口**来访问设备）

Linux系统主要部分：**内核、shell、文件系统、应用程序**

- **内核、shell和文件系统**一起形成了基本的操作系统结构，它们使得用户可以运行程序、管理文件并使用系统
- 分层设计的思想让程序间松耦合，有助于适配各种平台
- 驱动的**上面是系统调用**，**下面是硬件**
  ![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/3fb8de68383e5f7bf520f21242c455a0.png)

## 二、驱动分类

Linux驱动分为三个基础大类：**字符设备驱动，块设备驱动，网络设备驱动**。

1. 字符设备(Char Device)
   - 字符(char)设备是个能够像字节流（类似文件）一样被访问的设备。
   - 对字符设备发出读/写请求时，实际的硬件I/O操作一般紧接着发生。
   - 字符设备驱动程序通常至少要实现open、close、read和write系统调用。
   - 比如我们常见的lcd、触摸屏、键盘、led、串口等等，他们一般对应具体的硬件都是进行出具的采集、处理、传输。
2. 块设备(Block Device)
   - 一个块设备驱动程序主要通过传输固定大小的数据（一般为512或1k）来访问设备。
   - 块设备通过buffer cache(内存缓冲区)访问，可以随机存取，即：任何块都可以读写，不必考虑它在设备的什么地方。
   - 块设备可以通过它们的设备特殊文件访问，但是更常见的是通过文件系统进行访问。
   - 只有一个块设备可以支持一个安装的文件系统。
   - 比如我们常见的电脑硬盘、SD卡、U盘、光盘等。
3. 网络设备(Net Device)
   - 任何网络事务都经过一个网络接口形成，即一个能够和其他主机交换数据的设备。
   - 访问网络接口的方法仍然是给它们分配一个唯一的名字（比如eth0），但这个名字在文件系统中不存在对应的节点。
   - 内核和网络设备驱动程序间的通信，完全不同于内核和字符以及块驱动程序之间的通信，内核调用一套和数据包传输相关的函（socket函数）而不是read、write等。
   - 比如我们常见的网卡设备、蓝牙设备。

## 三、驱动程序的功能

1. 对设备初始化和释放
2. 把数据从内核传送到硬件和从硬件读取数据
3. 读取应用程序传送给设备文件的数据和回送应用程序请求的数据
4. 检测和处理设备出现的错误

## 四、驱动开发前提知识

### 4.1 内核态和用户态

- Kernel Mode（内核态）
  - 内核模式下（执行内核空间的代码），代码具有对硬件的所有控制权限。可以执行所有CPU指令，可以访问任意地址的内存
- User Mode（用户态）
  - 在用户模式下（执行用户空间的代码），代码没有对硬件的直接控制权限，也不能直接访问地址的内存。
  - 只能访问映射其地址空间的页表项中规定的在用户态下可访问页面的虚拟地址。
  - 程序是通过调用系统接口(System Call APIs)来达到访问硬件和内存

**Linux利用CPU实现内核态和用户态**

- ARM：内核态（svc模式），用户态（usr模式）
- x86 : 内核态（ring 0 ），用户态（ring 3）// x86有ring 0 - ring3四种特权等级

**Linux实现内核态和用户态切换**

- ARM Linux的系统调用实现原理是采用swi软中断从用户态切换至内核态
- X86是通过int 0x80中断进入内核态

Linux只能通过**系统调用**和**硬件中断**从用户空间进入内核空间

- 执行**系统调用**的内核代码运行在**进程上下文中**，他代表调用进程执行操作，因此能够**访问进程地址空间的所有数据**
- 处理**硬件中断**的内核代码运行在**中断上下文中**，他和**进程是异步**的，与任何一个特定进程无关通常，一个驱动程序模块中的某些函数作为系统调用的一部分，而其他函数负责中断处理

### 4.2 Linux下应用程序调用驱动程序流程

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/1114b0b5a6481fff5da2ad762f1aa811.png)

- Linux下进行驱动开发，完全将驱动程序与应用程序隔开，中间通过**C标准库函数**以及**系统调用**完成驱动层和应用层的数据交换。
- 驱动加载成功以后会在“/dev”目录下生成一个相应的文件，应用程序通过**对“/dev/xxx” (xxx 是具体的驱动文件名字) 的文件进行相应的操作**即可实现对硬件的操作。
- 用户空间不能直接对内核进行操作，因此必须使用一个叫做 **“系统调用”的方法** 来实现从用户空间“陷入” 到内核空间，这样才能实现对底层驱动的操作
- 每一个**系统调用**，在驱动中都有**与之对应的一个驱动函数**，在 Linux 内核文件 include/linux/fs.h 中有个叫做 **file_operations 的结构体**，此**结构体就是 Linux 内核驱动操作函数集合**。
  ![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/95ac162bffbd7df4a3411475a0b58934.png)

#### **大致流程**

1. 加载一个驱动模块，产生一个**设备文件**，有**唯一对应的inode结构体**
2. 应用层调用**open函数**打开设备文件，对于上层open调用到内核时会发生一次**软中断**，从用户空间**进入到内核空间**。
3. open会调用到**sys_open(内核函数)**，sys_open根据**文件的地址**，找到设备文件对应的**struct inode结构体**描述的信息，可以知道接下来要操作的设备类型（字符设备还是块设备），还会**分配一个struct file结构体**。
4. 根据**struct inode结构体里面记录的主设备号和次设备号**，在**驱动链表**(管理所有设备的驱动)里面，根据找到**字符设备驱动**
5. 每个字符设备都有一个**struct cdev结构体**。此结构体描述了字符设备所有信息，其中最重要的一项就是字符设备的**操作函数接口**
6. **找到struct cdev结构体**后，linux内核就会将struct cdev结构体所在的**内存空间首地址**记录在**struct inode结构体i_cdev成员**中，将struct cdev结构体中的记录的函**数操作接口地址记录**在**struct file结构体的f_ops成员中**。
7. 执行**xxx_open驱动函数**。

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/2dcbfda1e8dd7df5898c9990b883a7c4.png)

### 4.3 内核模块

**Linux 驱动有两种运行方式**

- 将**驱动编译进 Linux 内核中**，当 Linux 内核启动的时就会自动运行驱动程序。

- 将

  驱动编译成模块

  (Linux 下模块

  扩展名为.ko

  )，在Linux 内核启动以后使用相应命令加载驱动模块。

  - 内核模块是Linux内核向外部提供的一个插口
  - 内核模块是具有独立功能的程序，他可以被单独编译，但不能单独运行。他在运行时被链接到内核作为内核的一部分在内核空间运行
  - 内核模块便于驱动、文件系统等的二次开发

#### 内核模块组成

1. 模块加载函数

   ```cpp
   module_init(xxx_init);
   1
   ```

   - module_init 函数用来向 Linux 内核注册一个模块加载函数，
   - 参数 xxx_init 就是需要注册的具体函数（理解是模块的构造函数）
   - 当加载驱动的时， xxx_init 这个函数就会被调用

2. 模块卸载函数

   ```cpp
   module_exit(xxx_exit);
   1
   ```

   - module_exit函数用来向 Linux 内核注册一个模块卸载函数，
   - 参数 xxx_exit 就是需要注册的具体函数（理解是模块的析构函数）
   - 当使用“rmmod”命令卸载具体驱动的时候 xxx_exit 函数就会被调用

3. 模块许可证明

   ```cpp
   MODULE_LICENSE("GPL") //添加模块 LICENSE 信息 ,LICENSE 采用 GPL 协议
   1
   ```

4. 模块参数（可选）
   模块参数是一种内核空间与用户空间的交互方式，只不过是用户空间 --> 内核空间单向的，他对应模块内部的全局变量

5. 模块信息（可选）

   ```cpp
   MODULE_AUTHOR("songwei") //添加模块作者信息
   1
   ```

6. 模块打印 printk
   **printk在内核中用来记录日志信息的函数**，只能在内核源码范围内使用。和printf非常相似。
   printk函数主要做两件事情：**①将信息记录到log中 ②调用控制台驱动来将信息输出**

- printk 可以根据日志级别对消息进行分类，一共有 **8 个日志级别**

  ```cpp
  #define KERN_SOH  "\001" 
  #define KERN_EMERG KERN_SOH "0"  /* 紧急事件，一般是内核崩溃 */
  #define KERN_ALERT KERN_SOH "1"  /* 必须立即采取行动 */
  #define KERN_CRIT  KERN_SOH "2"  /* 临界条件，比如严重的软件或硬件错误*/
  #define KERN_ERR  KERN_SOH "3"  /* 错误状态，一般设备驱动程序中使用KERN_ERR 报告硬件错误 */
  #define KERN_WARNING KERN_SOH "4"  /* 警告信息，不会对系统造成严重影响 */
  #define KERN_NOTICE  KERN_SOH "5"  /* 有必要进行提示的一些信息 */
  #define KERN_INFO  KERN_SOH "6"  /* 提示性的信息 */
  #define KERN_DEBUG KERN_SOH "7"  /* 调试信息 */
  123456789
  ```

- 以下代码就是设置“gsmi: Log Shutdown Reason\n”这行消息的级别为 KERN_EMERG。

  ```cpp
  printk(KERN_DEBUG"gsmi: Log Shutdown Reason\n");
  1
  ```

  如果使用 printk 的时候不显式的设置消息级别，那 么printk 将会采用**默认级别MESSAGE_LOGLEVEL_DEFAULT，默认为 4**。

- 在 include/linux/printk.h 中有个宏 CONSOLE_LOGLEVEL_DEFAULT，定义如下：

  ```cpp
  #define CONSOLE_LOGLEVEL_DEFAULT 7
  1
  ```

  CONSOLE_LOGLEVEL_DEFAULT 控制着哪些级别的消息可以显示在控制台上，此宏默认为 7，意味着只有优先级高于 7 的消息才能显示在控制台上。

  这个就是 printk 和 printf 的最大区别，可以通过消息级别来决定哪些消息可以显示在控制台上。默认消息级别为 4，4 的级别比 7 高，所示直接使用 printk 输出的信息是可以显示在控制台上的。

#### 模块操作命令

1. 加载模块
   - insmod XXX.ko
     - 为模块分配内核内存、将模块代码和数据装入内存、通过内核符号表解析模块中的内核引用、调用模块初始化函数（module_init）
     - insmod要加载的模块有依赖模块，且其依赖的模块尚未加载，那么该insmod操作将失败
   - modprobe XXX.ko
     - 加载模块时会同时加载该模块所依赖的其他模块，**提供了模块的依赖性分析、错误检查、错误报告**
     - modprobe 提示无法打开“modules.dep”这个文件 ，输入 depmod 命令即可自动生成 modules.dep
2. 卸载模块
   - **rmmod XXX.ko**
3. 查看模块信息
   - lsmod
     - 查看系统中加载的所有模块及模块间的依赖关系
   - modinfo （模块路径）
     - 查看详细信息，内核模块描述信息，编译系统信息

### 4.4 设备号

- Linux 中每个设备都有一个设备号，设备号由主设备号和次设备号两部分组成
- 主设备号表示某一个具体的驱动，次设备号表示使用这个驱动的各个设备。
- Linux 提供了一个名为 dev_t 的数据类型表示设备号其中高 12 位为主设备号， 低 20 位为次设备
- 使用"cat /proc/devices"命令即可查看当前系统中所有已经使用了的设备号（主）

```cpp
MAJOR // 用于从 dev_t 中获取主设备号，将 dev_t 右移 20 位即可。
MINOR //用于从 dev_t 中获取次设备号，取 dev_t 的低 20 位的值即可。
MKDEV //用于将给定的主设备号和次设备号的值组合成 dev_t 类型的设备号。
123
```

### 4.5 地址映射

#### MMU(Memory Manage Unit)内存管理单元

1. 完成虚拟空间到物理空间的映射
2. 内存保护，设置存储器的访问权限，设置虚拟存储空间的缓冲特性
3. 对于 32 位的处理器来说，虚拟地址(VA,Virtual Address)范围是 2^32=4GB
   ![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/2cf0f751ef38a685c349790b9d110507.png)

#### 内存映射函数

CPU只能访问虚拟地址，**不能直接向寄存器地址写入数据，必须得到寄存器物理地址在Linux系统中对应的虚拟地址**。

物理内存和虚拟内存之间的转换，需要用到： ioremap 和 iounmap两个函数

- ioremap，用于获取**指定物理地址空间对应的虚拟地址空间**

  ```cpp
  /*
  phys_addr：要映射给的物理起始地址(cookie)
  size：要映射的内存空间大小
  mtype： ioremap 的类型，可以选择 MT_DEVICE、 MT_DEVICE_NONSHARED、MT_DEVICE_CACHED 和 MT_DEVICE_WC， 
  ioremap 函数选择 MT_DEVICE
  返回值： __iomem 类型的指针，指向映射后的虚拟空间首地址
  */
  #define ioremap(cookie,size) __arm_ioremap((cookie), (size),MT_DEVICE)
   
  void __iomem * __arm_ioremap(phys_addr_t phys_addr, size_t size, unsigned int mtype)
  {
      return arch_ioremap_caller(phys_addr, size, mtype, __builtin_return_address(0));
  }
  12345678910111213
  ```

  例：获取某个寄存器对应的虚拟地址

  ```cpp
  #define addr (0X020E0068)  // 物理地址
  static void __iomem*  va; //指向映射后的虚拟空间首地址的指针
  va=ioremap(addr, 4);   // 得到虚拟地址首地址
  123
  ```

- iounmap，卸载驱动使用 iounmap 函数释放掉 ioremap 函数所做的映射。
  参数 addr：要取消映射的虚拟地址空间首地址

  ```cpp
  iounmap(va);
  1
  ```

#### I/O内存访问函数

当**外部寄存器**或**外部内存映**射到内存空间时，称为 I/O 内存。但是对于 ARM 来说没有 I/O 空间，因此 ARM 体系下只有 I/O 内存(可以直接理解为内存)。

使用 ioremap 函数将寄存器的物理地址映射到虚拟地址后，可以直接通过指针访问这些地址，但是 Linux 内核不建议这么做，而是**推荐使用一组操作函数来对映射后的内存进行读写操作**。

- **读操作函数**

  ```cpp
  u8 readb(const volatile void __iomem *addr)
  u16 readw(const volatile void __iomem *addr)
  u32 readl(const volatile void __iomem *addr)
  123
  ```

  readb、 readw 和 readl 分别对应 8bit、 16bit 和 32bit 读操作，参数 addr 就是要读取写内存地址，返回值是读取到的数据

- **写操作函数**

  ```cpp
  void writeb(u8 value, volatile void __iomem *addr)
  void writew(u16 value, volatile void __iomem *addr)
  void writel(u32 value, volatile void __iomem *addr)
  123
  ```

  writeb、 writew 和 writel分别对应 8bit、 16bit 和 32bit 写操作，参数 value 是要写入的数值， addr 是要写入的地址。

## 五、设备树

Device Tree是一种**描述硬件的数据结构**，以便于**操作系统的内核可以管理和使用这些硬件**，包括CPU或CPU，内存，总线和其他一些外设。

Linux内核从3.x版本之后开始支持使用设备树，可以实现**驱动代码与设备的硬件信息相互的隔离**，减少了代码中的耦合性

- **引入设备树之前**：一些与硬件设备相关的具体信息都要写在驱动代码中，如果**外设发生相应的变化，那么驱动代码就需要改动**。
- **引入设备树之后**：通过设备树对硬件信息的抽象，驱动代码只要负责处理逻辑，而关于设备的具体信息存放到设备树文件中。如果只是硬件接口信息的变化而没有驱动逻辑的变化，开发者**只需要修改设备树文件信息，不需要改写驱动代码**。

### 5.1 DTS、DTB和DTC

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/08ccae554e82fb18fc2b2e1cc12d1af0.png)

- DTS
  - 设备树源码文件，硬件的相应信息都会写在.dts为后缀的文件中，每一款硬件可以单独写一份xxxx.dts
- DTSI
  - 对于一些**相同的dts配置可以抽象到dtsi文件**中，然后可以用**include的方式到dts文件**中
  - 同一芯片可以做一个dtsi，不同的板子不同的dts，然后include同一dtsi
  - 对于同一个节点的设置情况，dts中的配置会覆盖dtsi中的配置
- DTC
  - dtc是编译dts的工具
- DTB
  - dts经过dtc编译之后会得到dtb文件，设备树的二进制执行文件
  - dtb通过Bootloader引导程序加载到内核。

### 5.2 设备树框架

```cpp
1.根节点：\

2.设备节点：nodex

        ①节点名称：node

        ②节点地址：node@0, @后面即为地址

3.属性：属性名称（Property   name)和属性值(Property value)

4.标签
1234567891011
```

- “/”是根节点，每个设备树文件只有一个根节点。在设备树文件中会发现有的文件下也有“/”根节点，这两个**“/”根节点的内容会合并成一个根节点。**
- Linux 内核启动的时会解析设备树中各个节点的信息，并且在**根文件系统的/proc/devicetree 目录下根据节点名字创建不同文件夹**

### 5.3 DTS语法

#### **dtsi头文件**

```cpp
#include <dt-bindings/input/input.h>
#include "imx6ull.dtsi"
12
```

设备树也支持头文件，设备树的头文件扩展名为.dtsi。在.dts 设备树文件中，还可以通过“#include”来引用.h、 .dtsi 和.dts 文件。

#### **设备节点**

- 设备树是采用树形结构来描述板子上的设备信息的文件，**每个设备都是一个节点，叫做设备节点**，

- 每个节点都通过一些属性信息来描述节点信息，**属性就是键—值对**。

  ```cpp
  label: node-name@unit-address
  label:节点标签，方便访问节点：通过&label访问节点，追加节点信息
  node-name：节点名字，为字符串，描述节点功能
  unit-address：设备的地址或寄存器首地址，若某个节点没有地址或者寄存器，可以省略
  1234
  ```

- 设备树源码中常用的几种数据形式

  ```cpp
  1.字符串:  compatible = "arm,cortex-a7";设置 compatible 属性的值为字符串“arm,cortex-a7”
  2.32位无符号整数：reg = <0>; 设置reg属性的值为0
  3.字符串列表：字符串和字符串之间采用“,”隔开
  compatible = "fsl,imx6ull-gpmi-nand", "fsl, imx6ul-gpmi-nand";
  设置属性 compatible 的值为“fsl,imx6ull-gpmi-nand”和“fsl, imx6ul-gpmi-nand”。
  12345
  ```

#### **属性**

- **compatible属性（兼容属性）**
  `cpp "manufacturer,model" manufacturer：厂商名称 model：模块对应的驱动名字`
  **例：**
  imx6ull-alientekemmc.dts 中 sound 节点是 音频设备节点，采用的欧胜(WOLFSON)出品的 WM8960， sound 节点的 compatible 属性值如下：
  `cpp compatible = "fsl,imx6ul-evk-wm8960","fsl,imx-audio-wm8960";`
  属性值有两个，分别为“fsl,imx6ul-evk-wm8960”和“fsl,imx-audio-wm8960”，其中“fsl”表示厂商是飞思卡尔，“imx6ul-evk-wm8960”和“imx-audio-wm8960”表示驱动模块名字。

  sound这个设备**首先使用第一个兼容值在 Linux 内核里面查找**，看看能不能找到与之匹配的驱动文件，如果**没有找到的话就使用第二个兼容值查。**

  一般驱动程序文件会有一个 **OF 匹配表**，此 OF 匹配表**保存着一些 compatible 值**，如果**设备节点的 compatible 属性值和 OF 匹配表中的任何一个值相等**，那么就表示**设备可以使用这个驱动**。

  **在根节点来说**，**Linux 内核会通过根节点的 compoatible 属性查看是否支持此设备**，如果支持的话设备就会启动 Linux 内核。如果不支持的话那么这个设备就没法启动 Linux 内核。

- **model属性**
  model 属性值是一个字符串，一般 model 属性**描述设备模块信息**。

- **status属性**
  status 属性和**设备状态有关的**， status 属性值是字符串，描述设备的状态信息。
  ![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/540c7d513a5f6f15b853f5f1fb8939dc.png)

- **#address-cells 和#size-cells 属性**

  用于**描述子节点的地址信息**,reg属性的address 和 length的字长。

  - **#address-cells 属性**值决定了子节点 reg 属性中**地址信息所占用的字长**(32 位)，
  - **#size-cells 属性**值决定了子节点 reg 属性中**长度信息所占的字长**(32 位)。
  - 子节点的地址信息描述来自于父节点的#address-cells 和#size-cells的值，而不是该节点本身的值（当前节点的信息是描述子节点的，自己的信息在父节点里）

  ```cpp
  //每个“address length”组合表示一个地址范围，
  //其中 address 是起始地址， length 是地址长度，
  //#address-cells 表明 address 这个数据所占用的字长，
  // #size-cells 表明 length 这个数据所占用的字长.
  reg = <address1 length1 address2 length2 address3 length3……>
  12345
  ```

- **reg属性**
  reg 属性一般用于**描述设备地址空间资源信息**，一般都是某个外设的寄存器地址范围信息, reg 属性的值一般是(address， length)对.

  **例**

  ```cpp
  uart1: serial@02020000 {
      compatible = "fsl,imx6ul-uart",
          "fsl,imx6q-uart", "fsl,imx21-uart";
      reg = <0x02020000 0x4000>;
      interrupts = <GIC_SPI 26 IRQ_TYPE_LEVEL_HIGH>;
      clocks = <&clks IMX6UL_CLK_UART1_IPG>,
          <&clks IMX6UL_CLK_UART1_SERIAL>;
      clock-names = "ipg", "per";
      status = "disabled";
  };
  12345678910
  ```

  uart1 的**父节点 aips1:** aips-bus@02000000 设置了#address-cells = <1>、 #sizecells = <1>，因此 reg 属性中 address=0x02020000， length=0x4000。都是字长为1.

- **ranges属性**

  - ranges属性值可以为**空**或者**按照( child-bus-address , parent-bus-address , length )格式编写的数字**
  - ranges 是一个**地址映射/转换表**， ranges 属性每个项目由**子地址、父地址和地址空间长度**这三部分组成。
  - 如果 ranges 属性值为**空值**，说明**子地址空间和父地址空间完全相同，不需要进行地址转换**。

  ```cpp
  child-bus-address：子总线地址空间的物理地址，由父节点的#address-cells 确定此物理地址所占用的字长
  parent-bus-address： 父总线地址空间的物理地址，同样由父节点的#address-cells 确定此物理地址所占用的字长
  length： 子地址空间的长度，由父节点的#size-cells 确定此地址长度所占用的字长
  123
  ```

- **特殊节点**

  在**根节点“/”中**有两个特殊的子节点： **aliases 和 chosen**

  1. aliases

     ```cpp
     aliases {
         can0 = &flexcan1;
         can1 = &flexcan2;
         ...
         usbphy0 = &usbphy1;
         usbphy1 = &usbphy2;
     };
     1234567
     ```

     aliases 节点的主要功能就是**定义别名**，定义别名的目的就是为了方便访问节点。

     但是，一般会在**节点命名的时候会加上 label**，然后通过&label来访问节点。

  2. chosen
     chosen 不是一个真实的设备， **chosen 节点主要是为了 uboot 向 Linux 内核传递数据(bootargs 参数)。**

### 5.4 OF操作函数

Linux 内核提供了**一系列的函数来获取设备树中的节点或者属性信息**，这一系列的函数都有一个**统一的前缀“of_” (称为OF 函数）**

#### 查找节点

Linux 内核使用 **device_node 结构体来描述一个节点**：

```cpp
struct device_node {
    const char *name; /* 节点名字 */
    const char *type; /* 设备类型 */
    phandle phandle;
    const char *full_name; /* 节点全名 */
    struct fwnode_handle fwnode;
 
    struct property *properties; /* 属性 */
    struct property *deadprops; /* removed 属性 */
    struct device_node *parent; /* 父节点 */
    struct device_node *child; /* 子节点
    ...
}
12345678910111213
```

- 通过**节点名字**查找指定的节点：of_find_node_by_name

  ```cpp
  struct device_node *of_find_node_by_name(struct device_node *from,const char *name)
  1
  ```

  from：开始查找的节点，如果为 NULL 表示从根节点开始查找整个设备树。
  name：要查找的节点名字。
  返回值： 找到的节点，如果为 NULL 表示查找失败。

- 通过 **device_type 属性**查找指定的节点：of_find_node_by_type

  ```cpp
  struct device_node *of_find_node_by_type(struct device_node *from, const char *type)
  1
  ```

  from：开始查找的节点，如果为 NULL 表示从根节点开始查找整个设备树。
  type：要查找的节点对应的 type 字符串， device_type 属性值。
  返回值： 找到的节点，如果为 NULL 表示查找失败

- 通过**device_type 和 compatible两个属性**查找指定的节点：of_find_compatible_node

  ```cpp
  struct device_node *of_find_compatible_node(struct device_node *from,
                                              const char *type,
                                              const char *compatible)
  123
  ```

  from：开始查找的节点，如果为 NULL 表示从根节点开始查找整个设备树。
  type：要查找的节点对应的 type 字符串，device_type 属性值，可以为 NULL
  compatible： 要查找的节点所对应的 compatible 属性列表。
  返回值： 找到的节点，如果为 NULL 表示查找失败

- 通过 **of_device_id 匹配表**来查找指定的节点：of_find_matching_node_and_match

  ```cpp
  struct device_node *of_find_matching_node_and_match(struct device_node *from,
                                              const struct of_device_id *matches,
                                              const struct of_device_id **match)
  123
  ```

  from：开始查找的节点，如果为 NULL 表示从根节点开始查找整个设备树。
  matches： of_device_id 匹配表，在此匹配表里面查找节点。
  match： 找到的匹配的 of_device_id。
  返回值： 找到的节点，如果为 NULL 表示查找失败

- 通过**路径**来查找指定的节点：of_find_node_by_path

  ```cpp
  inline struct device_node *of_find_node_by_path(const char *path)
  1
  ```

  path：设备树节点中绝对路径的节点名，可以使用节点的别名
  返回值： 找到的节点，如果为 NULL 表示查找失败

#### 获取属性值

Linux 内核中使用**结构体 property** 表示属性

```cpp
struct property {
    char *name; /* 属性名字 */
    int length; /* 属性长度 */
    void *value; /* 属性值 */
    struct property *next; /* 下一个属性 */
    unsigned long _flags;
    unsigned int unique_id;
    struct bin_attribute attr;
}
123456789
```

- 查找**指定的属性**：of_find_property

  ```cpp
  property *of_find_property(const struct device_node *np,
                             const char *name,
                             int *lenp)
  1234
  ```

  np：设备节点。
  name： 属性名字。
  lenp：属性值的字节数，一般为NULL
  返回值： 找到的属性。

- 获取属性中**元素的数量(数组)**：of_property_count_elems_of_size

  ```cpp
  int of_property_count_elems_of_size(const struct device_node *np,
                                      const char *propname
                                      int elem_size)
  123
  ```

  np：设备节点。
  proname： 需要统计元素数量的属性名字。
  elem_size：元素长度。
  返回值： 得到的属性元素数量

- 从属性中**获取指定标号的 u32 类型数据值**:of_property_read_u32_index

  ```cpp
  int of_property_read_u32_index(const struct device_node *np,
                                  const char *propname,
                                  u32 index,
                                  u32 *out_value)
  1234
  ```

  np：设备节点。
  proname： 要读取的属性名字。
  index：要读取的值标号。
  out_value：读取到的值
  返回值： 0 读取成功;
  负值: 读取失败，
  -EINVAL 表示属性不存在
  -ENODATA 表示没有要读取的数据，
  -EOVERFLOW 表示属性值列表太小

- 读取**属性中 u8、 u16、 u32 和 u64 类型的数组数据**

  ```cpp
  of_property_read_u8_array
  of_property_read_u16_array 
  of_property_read_u32_array 
  of_property_read_u64_array 
  int of_property_read_u8_array(const struct device_node *np,
                                  const char *propname,
                                  u8 *out_values,
                                  size_t sz)
  12345678
  ```

  np：设备节点。
  proname： 要读取的属性名字。
  out_value：读取到的数组值，分别为 u8、 u16、 u32 和 u64。
  sz： 要读取的数组元素数量。
  返回值： 0：读取成功;
  负值: 读取失败
  -EINVAL 表示属性不存在
  -ENODATA 表示没有要读取的数据
  -EOVERFLOW 表示属性值列表太小

- 读取**属性中字符串值**：of_property_read_string

  ```cpp
  int of_property_read_string(struct device_node *np,
                              const char *propname,
                              const char **out_string)
  123
  ```

  np：设备节点。
  proname： 要读取的属性名字。
  out_string：读取到的字符串值。
  返回值： 0，读取成功，负值，读取失败

- 获取 **#address-cells** 属性值：of_n_addr_cells ，获取 **#size-cells** 属性值：of_size_cells 。

  ```cpp
  int of_n_addr_cells(struct device_node *np)
  int of_n_size_cells(struct device_node *np)
  12
  ```

  np：设备节点。
  返回值： 获取到的#address-cells 属性值。
  返回值： 获取到的#size-cells 属性值。

- 内存映射
  **of_iomap 函数用于直接内存映射**，前面通过 ioremap 函数来完成物理地址到虚拟地址的映射，采用设备树以后就可以直接通过 of_iomap 函数来获取内存地址所对应的虚拟地址。这样就**不用再去先获取reg属性值，再用属性值映射内存**。

  of_iomap 函数本质上也是将 reg 属性中地址信息转换为虚拟地址，如果 reg 属性有多段的话，可以通过 index 参数指定要完成内存映射的是哪一段， of_iomap 函数原型如下：

  ```cpp
  void __iomem *of_iomap(struct device_node *np,  int index)
  1
  ```

  np：设备节点。
  index： reg 属性中要完成内存映射的段，如果 reg 属性只有一段的话 index 就设置为 0。
  返回值： 经过内存映射后的虚拟内存首地址，如果为 NULL 的话表示内存映射失败。

  **例**

  ```cpp
  #if 1
  	/* 1、寄存器地址映射 */
  	IMX6U_CCM_CCGR1 = ioremap(regdata[0], regdata[1]);
  	SW_MUX_GPIO1_IO03 = ioremap(regdata[2], regdata[3]);
    	SW_PAD_GPIO1_IO03 = ioremap(regdata[4], regdata[5]);
  	GPIO1_DR = ioremap(regdata[6], regdata[7]);
  	GPIO1_GDIR = ioremap(regdata[8], regdata[9]);
  #else   //第一对：起始地址+大小 -->映射 这样就不用获取reg的值
  	IMX6U_CCM_CCGR1 = of_iomap(dtsled.nd, 0); 
  	SW_MUX_GPIO1_IO03 = of_iomap(dtsled.nd, 1);
    	SW_PAD_GPIO1_IO03 = of_iomap(dtsled.nd, 2);
  	GPIO1_DR = of_iomap(dtsled.nd, 3);
  	GPIO1_GDIR = of_iomap(dtsled.nd, 4);
  #endif
  1234567891011121314
  ```

#### of 函数在 led_init() 中应用

```cpp
	int ret;
    u32 regdate[14];
    const char *str;
	struct property *proper;
    /* 1 、获取设备节点： */
    dtb_led.nd = of_find_node_by_path("/songwei_led");
    if(dtb_led.nd == NULL)
    {
        printk("songwei_led node can not found!\r\n");
        return -EINVAL;
    }else
    {
        printk("songwei_led node has been found!\r\n");
    }

    /* 2 、获取 compatible  属性内容 */
    proper = of_find_property(dtb_led.nd ,"compatible",NULL);
    if(proper == NULL) 
    {
        printk("compatible property find failed\r\n");
    } else 
    {
        printk("compatible = %s\r\n", (char*)proper->value);
    }
    
    /* 3 、获取 status  属性内容 */
    ret = of_property_read_string(dtb_led.nd, "status", &str);
    if(ret < 0)
    {
        printk("status read failed!\r\n");
    }else 
    {
        printk("status = %s\r\n",str);
    }

    /* 4 、获取 reg  属性内容 */
    ret = of_property_read_u32_array(dtb_led.nd, "reg", regdate, 10);
    if(ret < 0) 
    {
        printk("reg property read failed!\r\n");
    }else 
    {
        u8 i = 0;
        printk("reg data:\r\n");
        for(i = 0; i < 10; i++)
            printk("%#X ", regdate[i]);
        printk("\r\n");
    }
123456789101112131415161718192021222324252627282930313233343536373839404142434445464748
```

## 六、字符设备驱动

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/9d54332265e3ab7127cbdcd3b9bc0d52.png)

### 6.1 字符设备基本驱动框架

#### 1.模块加载

```cpp
/* 驱动入口函数 */
static int __init xxx_init(void)
{
    /* 入口函数具体内容 */
    return 0;
}
/* 驱动出口函数 */
static void __exit xxx_exit(void)
{
    /* 出口函数具体内容 */
}
/* 将上面两个函数指定为驱动的入口和出口函数 */
module_init(xxx_init);
module_exit(xxx_exit)
1234567891011121314
```

#### 2.注册字符设备驱动

对于字符设备驱动而言，当驱动模块加载成功以后需要注册字符设备。卸载驱动模块的时也需要注销掉字符设备。
**字符设备的注册和注销函数原型：**

```cpp
static inline int register_chrdev(unsigned int major, 
									const char *name,
									const struct file_operations *fops)
 
static inline void unregister_chrdev(unsigned int major, 
									const char *name)
123456
```

这种注册函数会将后面所有的次设备号全部占用，而且主设备号需要我们自己去设置，现在不推荐这样使用。
一般**字符设备的注册**在**驱动模块的入口函数 xxx_init** 中进行，**字符设备的注销**在**驱动模块的出口函数 xxx_exit** 中进行。

#### 3.内存映射

- **内存映射**
  在Linux中不能直接访问寄存器，要想要操作寄存器需要完成物理地址到虚拟空间的映射。

```cpp
#define ioremap(cookie,size) __arm_ioremap((cookie), (size),
MT_DEVICE)
 
void __iomem * __arm_ioremap(phys_addr_t phys_addr,
						 	size_t size,
							unsigned int mtype)
{
    return arch_ioremap_caller(phys_addr, 
    							size, 
    							mtype,
    							__builtin_return_address(0));
}
123456789101112
```

返回值： **__iomem 是编辑器标记，指向映射后的虚拟空间首地址。**
建立映射：**映射的虚拟地址 = ioremap(IO内存起始地址，映射长度)**；
一旦映射成功，访问对应的虚拟地址就相当于访问对应的IO内存 。

- **解除映射**

```cpp
void iounmap (volatile void __iomem *addr)
1
```

#### 4.应用层和内核层传递数据

应用层和内核层是不能直接进行数据传输的。 要想进行数据传输， 要借助下面的这两个函数

```cpp
static inline long copy_from_user(void *to, const void __user * from, unsigned long n)
static inline long copy_to_user(void __user *to, const void *from, unsigned long n)
12
```

to：目标地址
from：源地址
n：将要拷贝数据的字节数
返回值：成功返回 0， 失败返回没有拷贝成功的数据字节数

#### 5. **字符设备最基本框架**

```cpp
#define CHRDEVBASE_MAJOR 200			//手动设置主设备号
#define CHRDEVBASE_NAME  "chrdevbase"	//设备名称
//内核缓存区
static char readbuf[100];						//读数据缓存
static char writebuf[100];						//写数据缓存
static char kerneldata[] = {"kernel data!"};	//测试数据
//硬件寄存器
#define GPIO_TEST_BASE (0x01234567) 	//宏定义寄存器映射地址
static void __iomem *GPIO_TEST;			// __iomem 类型的指针，指向映射后的虚拟空间首地址
//打开设备
static int chrdevbase_open(struct inode *inode, struct file *filp) 
{
	return 0;
}
// 从设备读取数据 
static ssize_t chrdevbase_read(struct file *filp , char __user *buf , size_t cnt , loff_t *offt) 
{
	int retvalue = 0;
	unsigned char databuf[1];
// 读取硬件寄存器
#if 0  
	//读取寄存器状态
	databuf[0] = readl(GPIO_TEST);
	retvalue = copy_to_user(buf , databuf, cnt);
//读取内核内存
#else	
	//测试数据拷贝到读数据缓存中
    memcpy(readbuf , kerneldata , sizeof(kerneldata));  
    //内核中数据（读缓存）拷贝到用户空间
    retvalue = copy_to_user(buf , readbuf , cnt);
#endif

    if(retvalue == 0) printk("kernel senddate ok!\n");   
  	else printk("kernel senddate failed!\n");
    return 0;
}
//向设备写数据 
static ssize_t chrdevbase_write(struct file *filp, const char __user *buf, size_t cnt , loff_t *offt) 
{
	int retvalue = 0;
//写硬件寄存器
#if 0
	writel(buf[0],GPIO_TEST);
//写内核缓存
#else
	//用户数据拷贝到内核空间（写缓存）
    retvalue = copy_from_user(writebuf , buf ,cnt);
#endif
    if(retvalue == 0) printk("kernel recevdate : %s\n",writebuf);
  	else printk("kernel recevdate failed!");
    return 0;
}
//关闭/释放设备
static int chrdevbase_release(struct inode *inode , struct file *filp) 
{
	return 0;
}
//设备操作函数
static struct file_operations chrdevbase_fops = {
    .owner = THIS_MODULE,
    .open = chrdevbase_open,
    .read = chrdevbase_read,
    .write = chrdevbase_write,
    .release = chrdevbase_release,
};
/* 驱动入口函数 */
static int __init chrdevbase_init(void)
{
	int retvalue = 0;
	//寄存器物理映射，物理地址映射到虚拟地址指针
	GPIO_TEST= ioremap(GPIO_TEST_BASE, 4);
	//注册字符设备驱动
    retvalue = register_chrdev(CHRDEVBASE_MAJOR, 	//主设备号
    							CHRDEVBASE_NAME, 	//设备名称
    							&chrdevbase_fops);	//设备操作函数集合
    							
    if(retvalue < 0) printk("chrdevbase driver register failed\n");
    printk("chrdevbase_init()\r\n");
    return 0;
}
/* 驱动出口函数 */
static void __exit chrdevbase_exit(void)
{
	//解除寄存器映射
	iounmap(GPIO_TEST);
	//注消字符设备驱动
	unregister_chrdev(CHRDEVBASE_MAJOR ,  	//主设备号
						CHRDEVBASE_NAME);	//设备名称
    printk("chrdevbase_exit()\r\n");
}
/* 将上面两个函数指定为驱动的入口和出口函数 */
module_init(chrdevbase_init);
module_exit(chrdevbase_exit);

MODULE_LICENSE("GPI");//GPL模块许可证
MODULE_AUTHOR("songwei");//作者信息
123456789101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596
```

#### 6. **创建驱动节点文件**

**加载驱动模块后，需手动创建驱动节点文件**

```cpp
mknod /dev/chrdevbase c 200 0
1
```

- 其中“mknod”是创建节点命令，
- “/dev/chrdevbase”是要创建的节点文件，
- “c”表示这是个字符设备，
- “200”是设备的主设备号，
- “0”是设备的次设备号。
- 创建完成以后就会存在/dev/chrdevbase 这个文件，可以使用“ls /dev/chrdevbase -l”命令查看

### 6.2 新字符设备基本驱动框架

上面的驱动框架，当使用 modprobe 加载驱动程序以后还需要使用命令mknod手动创建设备节点。

**在 Linux 下通过 udev（用户空间程序）** 来**实现设备文件的创建与删除**，但是在**嵌入式 Linux 中使用mdev** 来实现设备节点文件的自动创建与删除， Linux 系统中的**热插拔事件**也由 mdev 管理。

#### 1.**设备文件系统**

设备文件系统有devfs,mdev,udev这三种

1. **devfs**, 一个基于内核的动态设备文件系统

- devfs缺点（过时原因）
  - 不确定的设备映射
  - 没有足够的主/辅设备号
  - /dev目录下文件太多
  - 内核内存使用

1. **udev**，采用用户空间(user-space)工具来管理/dev/目录树，udev和文件系统分开

- udev和devfs的区别
  - 采用devfs，当一个并不存在的/dev节点被打开的时候，devfs能自动加载对应的驱动
  - udev的Linux应该在设备被发现的时候加载驱动模块，而不是当它被访问的时候
  - 系统中所有的设备都应该产生热拔插事件并加载恰当的驱动，而udev能注意到这点并且为它创建对应的设备节点。

1. **mdev**，是udev的简化版本，是busybox中所带的程序,适合用在嵌入式系统

#### 2.申请设备号

上述设备号为开发者挑选一个未使用的进行注册。Linux驱动开发推荐使用**动态分配设备号**。

- **动态申请设备号**

  ```cpp
  int alloc_chrdev_region(dev_t *dev, 
  						unsigned baseminor, 
  						unsigned count, 
  						const char *name)
  1234
  ```

  dev：保存申请到的设备号。
  baseminor： 次设备号起始地址，该函数可以申请一段连续的多个设备号，初始值一般为0
  count： 要申请的设备号数量。
  name：设备名字。

- **静态申请设备号**

  ```cpp
  int register_chrdev_region(dev_t from, unsigned count, const char *name);
  1
  ```

  from - 要申请的起始设备号
  count - 设备号个数
  name - 设备号在内核中的名称
  返回0申请成功，否则失败

- **释放设备号**

  ```cpp
  void unregister_chrdev_region(dev_t from, unsigned count)
  1
  ```

  from：要释放的设备号。
  count： 表示从 from 开始，要释放的设备号数量。

- **申请设备号模板**

  ```cpp
  //创建设备号 
  if (newchrled.major)   //定义了设备号就静态申请
  {		
  	newchrled.devid = MKDEV(newchrled.major, 0);
  	register_chrdev_region(newchrled.devid, 
  							NEWCHRLED_CNT, 
  							NEWCHRLED_NAME);
  } 
  else   //没有定义设备号就动态申请
  {		
   
  	alloc_chrdev_region(&newchrled.devid, 
  						0, 
  						NEWCHRLED_CNT, 
  						NEWCHRLED_NAME);//申请设备号 
  	newchrled.major = MAJOR(newchrled.devid);	//获取分配号的主设备号
  	newchrled.minor = MINOR(newchrled.devid);	// 获取分配号的次设备号
  }
  123456789101112131415161718
  ```

#### 3.注册字符设备

在 Linux 中使用 **cdev 结构体**表示一个字符设备

```cpp
struct cdev {
	struct kobject kobj;
	struct module *owner;
	const struct file_operations *ops;//操作函数集合
	struct list_head list;
	dev_t dev;//设备号
	unsigned int count;
};
12345678
```

在 cdev 中有两个重要的成员变量:**ops 和 dev**，字符设备文件操作函数集合**file_operations** 以及**设备号 dev_t**。

- **初始化cdev结构体变量**

  ```cpp
  void cdev_init(struct cdev *cdev, 
  				const struct file_operations *fops);
  12
  ```

  **例**

  ```cpp
  struct cdev testcdev;
  //设备操作函数
  static struct file_operations test_fops = {
      .owner = THIS_MODULE,
      //其他具体的初始项 
  };
  testcdev.owner = THIS_MODULE;
  //初始化 cdev 结构体变量
  cdev_init(&testcdev, &test_fops); 
  123456789
  ```

- **将设备添加到内核**

  cdev_add 函数用于向 Linux 系统添加字符设备(cdev 结构体变量)，首先使用 cdev_init 函数完成对 cdev 结构体变量的初始化，然后使用 **cdev_add 函数向 Linux 系统添加这个字符设备**。

  **将cdev添加到内核同时绑定设备号**。
  其实这里申请设备号和注册设备在第一中驱动中直接使用register_chrdev函数完成者两步操作

  ```cpp
  int cdev_add(struct cdev *p, dev_t dev, unsigned count)
  1
  ```

  p - 要添加的cdev结构
  dev - 绑定的起始设备号
  count - 设备号个数
  **例**

  ```cpp
  cdev_add(&testcdev, devid, 1); //添加字符设备
  1
  ```

- **将设备从内核注销**
  卸载驱动的时候一定要使用 **cdev_del 函数从 Linux 内核中删除**相应的字符设备

  ```cpp
  void cdev_del(struct cdev *p);
  1
  ```

  p - 要添加的cdev结构
  **例**

  ```cpp
  cdev_del(&testcdev); //删除 cdev
  1
  ```

#### 4.自动创建设备节点

上面的驱动框架，当使用 modprobe 加载驱动程序以后还需要使用命令mknod手动创建设备节点。

在驱动中实现**自动创建设备节点**的功能以后，使用 modprobe 加载驱动模块成功的话就会**自动在/dev 目录下创建**对应的设备文件。

自动创建设备节点的工作是在**驱动程序的入口函数中完成**的，一般在 **cdev_add 函数后面添加**自动创建设备节点相关代码。

- **创建一个class类**

  ```cpp
  struct class *class_create(struct module *owner, const char *name);
  1
  ```

  - class_create 一共有两个参数，参数 owner 一般为 THIS_MODULE，参数 name 是类名字。
  - 设备类名对应 /sys/class 目录的子目录名。
  - 返回值是个指向结构体 class 的指针，也就是创建的类。

- **删除一个class类**

  ```cpp
  void class_destroy(struct class *cls); // cls要删除的类
  1
  ```

- **创建设备**
  还需要在**类下创建一个设备**,使用 device_create 函数在类下面创建设备。
  **成功会在 /dev 目录下生成设备文件。**

  ```cpp
  struct device *device_create(struct class *class,
      						struct device *parent,
      						dev_t devt,
      						void *drvdata,
  							const char *fmt, ...)
  12345
  ```

  *class——设备类指针,
  *parent——父设备指针,
  devt——设备号,
  *drvdata——额外数据,
  *fmt——设备文件名

- **删除设备**
  卸载驱动的时候需要删除掉创建的设备

  ```cpp
  void device_destroy(struct class *class, dev_t devt);
  1
  ```

  class——设备所处的类
  devt——设备号

#### 5.文件私有数据

- 每个硬件设备都有一些属性，比如主设备号(dev_t)，类(class)、设备(device)，
- 一个设备的**所有属性信息将其做成一个结构体**,
- 编写驱动 **open 函数**的时候将设备结构体作为**私有数据添**加到设备文件中。
- 在 write、 read、 close 函数中**直接读取 private_data**即可得到设备结构体

```cpp
/* newchrled设备结构体 */
struct newchrled_dev{
	dev_t devid;			/* 设备号 	 */
	struct cdev cdev;		/* cdev 	*/
	struct class *class;		/* 类 		*/
	struct device *device;	/* 设备 	 */
	int major;				/* 主设备号	  */
	int minor;				/* 次设备号   */
};
struct newchrled_dev newchrled;	/* led设备 */
 
/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int led_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &newchrled; /* 设置私有数据 */
	return 0;
}
 
static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    struct newchrled_dev *dev = (struct newchrled_dev *)filp->private_data;
	return 0;
}
1234567891011121314151617181920212223242526272829
```

#### 6.新字符设备驱动程序框架

```cpp
#define NEWCHR_CNT 1
#define NEWCHR_NAME "NEWCHR"
//内核缓存区
static char readbuf[100];						//读数据缓存
static char writebuf[100];						//写数据缓存
static char kerneldata[] = {"kernel data!"};	//测试数据
//硬件寄存器
#define GPIO_TEST_BASE (0x01234567) 	//宏定义寄存器映射地址
static void __iomem *GPIO_TEST;			// __iomem 类型的指针，指向映射后的虚拟空间首地址

/* newchr设备结构体 */
struct newchr_dev{
	dev_t devid;			/* 设备号 	 */
	struct cdev cdev;		/* cdev 	*/
	struct class *class;		/* 类 		*/
	struct device *device;	/* 设备 	 */
	int major;				/* 主设备号	  */
	int minor;				/* 次设备号   */
};
 
struct newchrled_dev newchr;	/* newchr设备 */

//打开设备
static int chrdevbase_open(struct inode *inode, struct file *filp) 
{
	filp->private_data = &newchr; /* 设置私有数据 */
	return 0;
}
// 从设备读取数据 
static ssize_t chrdevbase_read(struct file *filp , char __user *buf , size_t cnt , loff_t *offt) 
{
	int retvalue = 0;
	unsigned char databuf[1];
	//读取私有数据
	struct newchr_dev *dev = (struct newchr_dev *)filp->private_data;
// 读取硬件寄存器
#if 0  
	//读取寄存器状态
	databuf[0] = readl(GPIO_TEST);
	retvalue = copy_to_user(buf , databuf, cnt);
//读取内核内存
#else	
	//测试数据拷贝到读数据缓存中
    memcpy(readbuf , kerneldata , sizeof(kerneldata));  
    //内核中数据（读缓存）拷贝到用户空间
    retvalue = copy_to_user(buf , readbuf , cnt);
#endif

    if(retvalue == 0) printk("kernel senddate ok!\n");   
  	else printk("kernel senddate failed!\n");
    return 0;
}
//向设备写数据 
static ssize_t chrdevbase_write(struct file *filp, const char __user *buf, size_t cnt , loff_t *offt) 
{
	int retvalue = 0;
	//读取私有数据
	struct newchr_dev *dev = (struct newchr_dev *)filp->private_data;
//写硬件寄存器
#if 0
	writel(buf[0],GPIO_TEST);
//写内核缓存
#else
	//用户数据拷贝到内核空间（写缓存）
    retvalue = copy_from_user(writebuf , buf ,cnt);
#endif
    if(retvalue == 0) printk("kernel recevdate : %s\n",writebuf);
  	else printk("kernel recevdate failed!");
    return 0;
}
//关闭/释放设备
static int chrdevbase_release(struct inode *inode , struct file *filp) 
{
	return 0;
}
//设备操作函数
static struct file_operations chrdevbase_fops = {
    .owner = THIS_MODULE,
    .open = chrdevbase_open,
    .read = chrdevbase_read,
    .write = chrdevbase_write,
    .release = chrdevbase_release,
};
/* 驱动入口函数 */
static int __init chrdevbase_init(void)
{
	int retvalue = 0;
	//寄存器物理映射，物理地址映射到虚拟地址指针
	GPIO_TEST= ioremap(GPIO_TEST_BASE, 4);
	
	//申请设备号
    if(newchr.major)		//静态申请
    {
        newchr.devid = MKDEV(newchr.major , 0);
        register_chrdev_region(newchr.devid, NEWCHR_CNT,NEWCHR_NAME);
    }else					//动态申请
    {
        alloc_chrdev_region(&newchr.devid , 0 , NEWCHR_CNT , NEWCHR_NAME);
        newchr.major = MAJOR(newchr.devid);
        newchr.minor = MINOR(newchr.devid);
    }   
    printk("newche major=%d,minor=%d\r\n",newchr.major , newchr.minor);

	//字符串设备初始化、注册添加到内核
	newchr.cdev.owner = THIS_MODULE;
    cdev_init(&newchr.cdev , &newchr_fops);
    cdev_add(&newchr.cdev , newchr.devid ,NEWCHR_LED_CNT);
	//创建设备类
    newchr.class = class_create(THIS_MODULE , NEWCHR_NAME);
    if(IS_ERR(newchr.class))
    {
        return PTR_ERR(newchr.class);
    }
	//创建类的实例化设备 ,dev下面创建文件
    newchr.device = device_create(newchr.class , NULL , newchr.devid ,NULL ,NEWCHR_NAME);
    if(IS_ERR(newchr.device))
    {
        return PTR_ERR(newchr.device);
    }
    return 0;
}
/* 驱动出口函数 */
static void __exit chrdevbase_exit(void)
{
	//解除寄存器映射
	iounmap(GPIO_TEST);
	//删除cdev字符串设备
	cdev_del(&newchr.cdev);
	//释放设备号
    unregister_chrdev_region(newchr.devid , NEWCHR_CNT);
	//具体设备注销
    device_destroy(newchr.class, newchr.devid);
    //类注销
    class_destroy(newchr.class);
}
/* 将上面两个函数指定为驱动的入口和出口函数 */
module_init(chrdevbase_init);
module_exit(chrdevbase_exit);

MODULE_LICENSE("GPI");//GPL模块许可证
MODULE_AUTHOR("songwei");//作者信息
123456789101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899100101102103104105106107108109110111112113114115116117118119120121122123124125126127128129130131132133134135136137138139140141
```

## 七、pinctrl子系统

Linux 内核提供了 **pinctrl 子系统和 gpio 子系统**用于 **GPIO 驱动**。

### 7.1 pinctrl 子系统主要工作内容：

- **获取设备树中 pin 信息**，**管理系统中所有的可以控制的 pin**， 在系统初始化的时候， 枚举所有可以控制的 pin， 并标识这些 pin
- 根据获取到的 pin 信息来**设置 pin 的复用功能**，对于 SOC 而言， 其引脚除了配置成普通的 GPIO 之外，**若干个引脚还可以组成一个 pin group**， 形成特定的功能
- 根据获取到的 pin 信息来**设置 pin 的电气特性**，比如上/下拉、速度、驱动能力等。

开发时只需要在**设备树**里面**设置好某个 pin 的相关属性即可**，其他的初始化工作均由 pinctrl 子系统来完成。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/cdabd87267e2651db68c6192c4cc747c.png)

### 7.2 pinctrl的设备树设置

在设备树里面创建一个节点来描述 PIN 的配置信息。pinctrl 子系统一般在**iomuxc子节点下**，所有需要配置用户自己的pinctrl需要在该节点下添加。
**例**

```cpp
iomuxc: iomuxc@020e0000 {
	compatible = "fsl,imx6ul-iomuxc";
	reg = <0x020e0000 0x4000>;
	pinctrl-names = "default";
	pinctrl-0 = <&pinctrl_hog_1>;
	imx6ul-evk {
		pinctrl_hog_1: hoggrp-1 {
			fsl,pins = <
				MX6UL_PAD_UART1_RTS_B__GPIO1_IO19 0x17059
				.......
			>;
	};
......
}
1234567891011121314
```

- compatible 属性值为“fsl,imx6ul-iomuxc” ，
- pinctrl_hog_1 子节点所使用的 PIN 配置信息，如UART1_RTS_B 的配置信息
  - UART1_RTS_B 这个 PIN 是作为 SD 卡的检测引脚
  - MX6UL_PAD_UART1_RTS_B__GPIO1_IO19，这是一个宏定义，表 示 将 UART1_RTS_B 这个 IO 复用为 GPIO1_IO19（复用属性）
    - 此宏定义后面跟着 5 个数字，0x0090 0x031C 0x0000 0x5 0x0，含义是<mux_reg conf_reg input_reg mux_mode input_val>
  - 0x17059 就是 conf_reg 寄存器值 ， 设置一个 IO 的上/下拉、驱动能力和速度（电气属性）

### 7.3 设备树中添加pinctrl模板

1. 添加pinctrl设备结点
   **同一个外设的 PIN 都放到一个节点**里面，在 **iomuxc 节点**中下添加“pinctrl_test”节点。**节点前缀一定要为“pinctrl_”。**

设备树是通过**属性来保存信息**的，因此需要**添加一个属性**，属性名字一定要为** fsl,pins **

```cpp
&iomuxc {
	pinctrl-names = "default";
	pinctrl-0 = <&pinctrl_hog_1>;
	imx6ul-evk {
		......
			pinctrl_led: ledgrp{
						fsl,pins = <
							MX6UL_PAD_GPIO1_IO03__GPIO1_IO03	0x10b0
						>;
				};
		......
	};
123456789101112
```

1. 添加具体设备节点，调用pinctrl信息

在根节点“/”下创建 LED 灯节点，节点名为“gpioled”
只需要关注gpioled设备节点下的**pinctrl-names 和 pinctrl-0 两条语句**， 这两句就是**引用**在 **iomuxc 中配置的 pinctrl 节点**

```cpp
test {
    pinctrl-names = "default"， "wake up";
    pinctrl-0 = <&pinctrl_test>;
    pinctrl-1 = <&pinctrl_test_2>;
    /* 其他节点内容 */
};
123456
```

- pinctrl-names = “default”， “wake up”; 设备的状态， 可以有多个状态， default 为状态 0， wake up 为状态 1。
- pinctrl-0 = <&pinctrl_test>;第 0 个状态所对应的引脚配置， 也就是 default 状态对应的引脚在 pin controller 里面定义好的节点 pinctrl_test里面的管脚配置。
- pinctrl-1 = <&pinctrl_test_2>;第 1 个状态所对应的引脚配置， 也就是 wake up 状态对应的引脚在 pin controller 里面定义好的节点 pinctrl_test_2里面的管脚配置。

**例**

```cpp
gpioled {
		#address-cells = <1>;
		#size-cells = <1>;
		compatible = "songwei-gpioled";
		pinctrl-names = "default";
		pinctrl-0 = <&pinctrl_led>;
		led-gpio = <&gpio1 3 GPIO_ACTIVE_LOW>;
		status = "okay";
	};
123456789
```

- pinctrl-names = “default”; 设备的状态， 可以有多个状态， default 为状态 0。
- pinctrl-0 属性设置 LED 灯所使用的 PIN 对应的 pinctrl 节点

## 八、GPIO子系统

当使用 pinctrl 子系统将引脚的复用设置为 GPIO，可以使用 **GPIO 子系统来操作GPIO**

### 8.1 GPIO子系统工作内容

通过 GPIO 子系统功能要实现：

- 引脚功能的配置（设置为 GPIO，GPIO 的方向， 输入输出模式，读取/设置 GPIO 的值）
- 实现软硬件的分离（分离出硬件差异， 有厂商提供的底层支持； 软件分层。 驱动只需要调用接口 API 即可操作 GPIO）
- iommu 内存管理（直接调用宏即可操作 GPIO）

### 8.2 GPIO子系统设备树设置

在具体设备节点中添加GPIO信息

```cpp
gpioled {
		#address-cells = <1>;
		#size-cells = <1>;
		compatible = "songwei-gpioled";
		pinctrl-names = "default";
		pinctrl-0 = <&pinctrl_led>;
		//gpio信息
		led-gpio = <&gpio1 3 GPIO_ACTIVE_LOW>;
		status = "okay";
	};
12345678910
```

- led-gpio 属性指定了 LED 灯所使用的 GPIO，在这里就是 GPIO1 的 IO03，低电平有效。
- 稍后编写驱动程序的时候会获取 led-gpio 属性的内容来得到 GPIO 编号，因为 gpio 子系统的 API 操作函数需要 GPIO 编号

### 8.3 API函数

1. gpio_request

   gpio_request 函数用于申请一个 GPIO 管脚，在使用一个 GPIO 之前一定要使用 gpio_request进行申请。

   ```cpp
   int gpio_request(unsigned gpio, const char *label)
   1
   ```

   - gpio：要申请的 gpio 标号，使用 of_get_named_gpio 函数从设备树获取指定 GPIO 属性信
     息，此函数会返回这个 GPIO 的标号
   - label：给 gpio 设置个名字。
   - 返回值： 0，申请成功；其他值，申请失败。

2. gpio_free
   如果不使用某个 GPIO ，需要调用 gpio_free 函数进行释放。

   ```cpp
   void gpio_free(unsigned gpio)；  // gpio：要释放的 gpio 标号。
   1
   ```

3. gpio_direction_input
   设置某个 GPIO 为输入

   ```cpp
   int gpio_direction_input(unsigned gpio) //gpio：要设置为输入的 GPIO 标号。
   1
   ```

4. gpio_direction_output
   设置某个 GPIO 为输出，并且设置默认输出值。

   ```cpp
   int gpio_direction_output(unsigned gpio, int value)
   1
   ```

   - gpio：要设置为输出的 GPIO 标号。
   - value： GPIO 默认输出值。
   - 返回值： 0，设置成功；负值，设置失败

5. gpio_get_value
   获取某个 GPIO 的值(0 或 1)

   ```cpp
   #define gpio_get_value __gpio_get_value
   int __gpio_get_value(unsigned gpio)
   12
   ```

   - gpio：要获取的 GPIO 标号。
   - 返回值： 非负值，得到的 GPIO 值；负值，获取失败

6. gpio_set_value
   设置某个 GPIO 的值

   ```cpp
   #define gpio_set_value __gpio_set_value
   void __gpio_set_value(unsigned gpio, int value)
   12
   ```

   - gpio：要设置的 GPIO 标号。
   - value： 要设置的值

### 8.4 GPIO相关OF函数

1. of_gpio_named_count
   获取设备树某个属性里面定义了几个GPIO 信息。

   ```cpp
   int of_gpio_named_count(struct device_node *np, const char *propname)
   1
   ```

   - np：设备节点。
   - propname：要统计的 GPIO 属性。
   - 返回值： 正值，统计到的 GPIO 数量；负值，失败

2. of_gpio_count
   此函数统计的是gpios属性的 GPIO 数量，而 of_gpio_named_count 函数可以统计任意属性的GPIO 信息

   ```cpp
   int of_gpio_count(struct device_node *np)
   1
   ```

   - np：设备节点。
   - 返回值： 正值，统计到的 GPIO 数量；负值，失败

3. of_get_named_gpio
   获取 GPIO 编号，在Linux 内核中关于 GPIO 的 API 函数都要使用 GPIO 编号，此函数会将设备树中类似<&gpio5 7 GPIO_ACTIVE_LOW>的属性信息转换为对应的 GPIO 编号。

   ```cpp
   int of_get_named_gpio(struct device_node *np,
                          const char *propname,
                          int index)
   123
   ```

   - np：设备节点。
   - propname：包含要获取 GPIO 信息的属性名。
   - index： GPIO 索引，因为一个属性里面可能包含多个 GPIO，此参数指定要获取哪个 GPIO的编号，如果只有一个 GPIO 信息的话此参数为 0。
   - 返回值： 正值，获取到的 GPIO 编号；负值，失败。

### 8.5 pinctrl和gpio子系统使用程序框架

```cpp
	int ret = 0;
    /* 1 、获取设备节点：alphaled */
    gpio_led.nd = of_find_node_by_path("/gpioled");
    if(gpio_led.nd == NULL)
    {
        printk("songwei_led node can not found!\r\n");
        return -EINVAL;
    }else
    {
        printk("songwei_led node has been found!\r\n");
    }
    /* 2、 获取设备树中的 gpio 属性，得到 LED 所使用的 LED 编号 */
    gpio_led.led_gpio = of_get_named_gpio(gpio_led.nd,"led-gpio",0);
    if(gpio_led.led_gpio < 0) 
    {
        printk("can't get led-gpio");
        return -EINVAL;
    }
    printk("led-gpio num = %d\r\n", gpio_led.led_gpio);
    /* 3、设置 GPIO1_IO03 为输出，并且输出高电平，默认关闭 LED 灯 */
    ret = gpio_direction_output(gpio_led.led_gpio, 1);
    if(ret < 0) 
    {
        printk("can't set gpio!\r\n");
    }
12345678910111213141516171819202122232425
```

## 九、内核并发与竞争

### 9.1 并发与竞争概念

Linux 系统是个多任务操作系统，会存在多个任务同时访问同一片内存区域，这些任务可能会相互覆盖这段内存中的数据，造成内存数据混乱。我们需要对**共享数据进行相应的保护处理**。

- **并发**：多个执行单元同时进行或多个执行单元微观串行执行，宏观并行执行。
  - 多线程并发访问， Linux 是多任务(线程)的系统，多线程访问是最基本的原因
  - 抢占式并发访问，Linux 内核支持抢占，调度程序可以在任意时刻抢占正在运行的线程，从而运行其他的线程。
  - 中断程序并发访问，硬件中断的权利可以是很大的。
  - SMP(多核)核间并发访问，多核 CPU 存在核间并发访问。
- **竞争**：并发的执行单元对共享资源（硬件资源和软件上的全局变量）的访问而导致的竞争状态。
- **临界资源：** 多个进程访问的资源，共享数据段
- **临界区**：多个进程访问的代码段

### 9.2 原子操作

**原子操作**是指**不能再进一步分割的操作**。一般原子操作用于整形变量或者位操作。

Linux 内核定义了叫做 **atomic_t 的结构体**来完成整形数据的原子操作，在使用中用**原子变量来代替整形变量**，此结构体定义在 include/linux/types.h 文件中

```cpp
typedef struct {
    int counter;
} atomic_t;
123
```

如果要使用原子操作 API 函数，首先要先定义一个 atomic_t 的变量，

```cpp
atomic_t a; //定义 a
atomic_t b = ATOMIC_INIT(0); //定义原子变量 b 并赋初值为 0
12
```

**原子操作API函数**
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/301b5f50777c454537e19a6e2e59b80b.png)
**原子位操作 API 函数**
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/19c7eefab4e0fb1a1e5eacd70e8b05e4.png)
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/f19962c88130e7903276ef98fb552bca.png)
**例**

- 原子变量 lock，用来实现一次只能允许一个应用访问 LED 灯，led_init 驱动入口函数会将 lock 的值设置为 1。
- 每次打开驱动设备的时候先使用 atomic_dec_and_test 函数将 lock 减 1，如果 atomic_dec_and_test函数返回值为真就表示 lock 当前值为 0，说明设备可以使用
- 如果 atomic_dec_and_test 函数返回值为假，就表示 lock 当前值为负数(lock 值默认是 1),lock 值为负数的可能性只有一个，那就
  是其他设备正在使用 LED,退出之前调用函数 atomic_inc 将 lock 加 1,因为此时 lock 的值被减成了负数，必须要对其加 1，将 lock 的值
  变为 0。

```cpp
/* gpioled设备结构体 */
struct gpioled_dev{
	......
	int led_gpio;			/* led所使用的GPIO编号		*/
	atomic_t lock;			/* 原子变量 */
};

struct gpioled_dev gpioled;	/* led设备 */

static int led_open(struct inode *inode, struct file *filp)
{
	/* 通过判断原子变量的值来检查LED有没有被别的应用使用 */
	if (!atomic_dec_and_test(&gpioled.lock)) 
	{
		atomic_inc(&gpioled.lock);	/* 小于0的话就加1,使其原子变量等于0 */
		return -EBUSY;				/* LED被使用，返回忙 */
	}
	......
}

static int led_release(struct inode *inode, struct file *filp)
{
	struct gpioled_dev *dev = filp->private_data;

	/* 关闭驱动文件的时候释放原子变量 */
	atomic_inc(&dev->lock);
	return 0;
}


static int __init led_init(void)
{
	int ret = 0;

	/* 初始化原子变量 */
	atomic_set(&gpioled.lock, 1);	/* 原子变量初始值为1 */
	......
	return 0;
}

static void __exit led_exit(void)
{
	......
}

123456789101112131415161718192021222324252627282930313233343536373839404142434445
```

### 9.2 自旋锁

对于自旋锁而言，如果自旋锁正在被线程 A 持有，线程 B 想要获取自旋锁，那么线程 B 就会处于忙循环-旋转-等待状态，线程 B 不会进入休眠状态或者说去做其他的处理，直到线程A释放自旋锁，线程B才可以访问共享资源。

由于**等待自旋锁会浪费处理器时间**，降低系统性能，所以自旋锁的持有时间不能太长，所以自旋锁**适用于短时期的轻量级加锁**。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/35ddd4f9489f718b079c81810f3b92c7.png)
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/b3414b6d78fb1f6d151024d5d84370bf.png)
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/02c6e7d98d28f48ee4e0fc9d061f21a9.png)

**例**

- dev_stats 表示设备状态，如果为 0 的话表示设备还没有被使用，如果大于 0 的话就表示设备已经被使用了。
- 调用 spin_lock_irqsave 函数获取锁，为了考虑到驱动兼容性，这里并没有使用 spin_lock 函数来获取锁。
- 判断dev_stats 是否大于 0，如果是的话表示设备已经被使用了，那么就调用 spin_unlock_irqrestore函数释放锁，并且返回-EBUSY。
- 如果设备没有被使用的话就在将 dev_stats 加 1，表示设备要被使用了，然后调用 spin_unlock_irqrestore 函数释放锁。
- 在 release 函数中将 dev_stats 减 1，表示设备被释放了
- 自旋锁的工作就是保护dev_stats 变量，真正实现对设备互斥访问的是 dev_stats。

```cpp
/* gpioled设备结构体 */
struct gpioled_dev{
	......
	int dev_stats;			/* 设备使用状态，0，设备未使用;>0,设备已经被使用 */
	spinlock_t lock;		/* 自旋锁 */
};

struct gpioled_dev gpioled;	/* led设备 */

static int led_open(struct inode *inode, struct file *filp)
{
	......
	spin_lock_irqsave(&gpioled.lock, flags);	/* 上锁 */
	if (gpioled.dev_stats) 
	{					/* 如果设备被使用了 */
		spin_unlock_irqrestore(&gpioled.lock, flags);/* 解锁 */
		return -EBUSY;
	}
	gpioled.dev_stats++;	/* 如果设备没有打开，那么就标记已经打开了 */
	spin_unlock_irqrestore(&gpioled.lock, flags);/* 解锁 */

	return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
	......
	/* 关闭驱动文件的时候将dev_stats减1 */
	spin_lock_irqsave(&dev->lock, flags);	/* 上锁 */
	if (dev->dev_stats) 
	{
		dev->dev_stats--;
	}
	spin_unlock_irqrestore(&dev->lock, flags);/* 解锁 */

	return 0;
}

/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init led_init(void)
{
	int ret = 0;
	/*  初始化自旋锁 */
	spin_lock_init(&gpioled.lock);
	......
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit led_exit(void)
{
	......
}

12345678910111213141516171819202122232425262728293031323334353637383940414243444546474849505152535455565758596061
```

### 9.3 信号量

Linux 内核提供了信号量机制，信号量常常用于控制对共享资源的访问。它是一个计数器，常用于实现进程间的互斥与同步，而不是用于存储进程间通信数据。

- 信号量可以使等待资源线程进入**休眠状态**，适用于占用资源比较久的场合
- 信号量会引起休眠，中断不能休眠，所以信号量不能用于中断。
- 如果共享资源的持有时间比较短，不适合使用信号量，因为频繁的休眠、切换线程引起的开销要远大于信号量带来的优势，此时使用自旋锁。
  ![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/003c89ebd5477289b66e45a82ebed5b3.png)

**例**

```cpp
/* gpioled设备结构体 */
struct gpioled_dev{
	......
	struct semaphore sem;	/* 信号量 */
};

struct gpioled_dev gpioled;	/* led设备 */

static int led_open(struct inode *inode, struct file *filp)
{
	......
	/* 获取信号量 */
	if (down_interruptible(&gpioled.sem)) { /* 获取信号量,进入休眠状态的进程可以被信号打断 */
		return -ERESTARTSYS;
	}
#if 0
	down(&gpioled.sem);		/* 不能被信号打断 */
#endif

	return 0;
}
static int led_release(struct inode *inode, struct file *filp)
{
	......
	up(&dev->sem);		/* 释放信号量，信号量值加1 */
	return 0;
}

/* 设备操作函数 */
static struct file_operations gpioled_fops = {
	......
};

static int __init led_init(void)
{
	int ret = 0;

	/* 初始化信号量 */
	sema_init(&gpioled.sem, 1);
	......
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit led_exit(void)
{
	......
}
123456789101112131415161718192021222324252627282930313233343536373839404142434445464748495051
```

### 9.4 互斥体

Linux 提供了专门的互斥体mutex **（等效信号量为1）** 。互斥访问表示一次只有一个线程可以访问共享资源，**不能递归申请互斥体**（死锁）。在 Linux 驱动的时遇到需要互斥访问的地方一般使用 mutex。

- mutex 可以导致休眠，因此不能在中断中使用 mutex，中断中只能使用自旋锁。
- mutex 保护的临界区可以调用引起阻塞的 API 函数(信号量也可以)
- 因为一次只有一个线程可以持有 mutex，所以，必须由 mutex 的持有者释放 mutex。并且 mutex 不能递归上锁和解锁
  ![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/a9b0647586385bdc490ea61b2e89eecd.png)

```cpp
/* gpioled设备结构体 */
struct gpioled_dev{
	......
	struct mutex lock;		/* 互斥体 */
};

struct gpioled_dev gpioled;	/* led设备 */

static int led_open(struct inode *inode, struct file *filp)
{
	......
	/* 获取互斥体,可以被信号打断 */
	if (mutex_lock_interruptible(&gpioled.lock)) {
		return -ERESTARTSYS;
	}
#if 0
	mutex_lock(&gpioled.lock);	/* 不能被信号打断 */
#endif
	return 0;
}
static int led_release(struct inode *inode, struct file *filp)
{
	......
	/* 释放互斥锁 */
	mutex_unlock(&dev->lock);
	return 0;
}

/* 设备操作函数 */
static struct file_operations gpioled_fops = {
	......
};

static int __init led_init(void)
{
	......
	/* 初始化互斥体 */
	mutex_init(&gpioled.lock);
	......
}

static void __exit led_exit(void)
{
	......
}
123456789101112131415161718192021222324252627282930313233343536373839404142434445
```

## 十、内核定时器

### 10.1 内核时间管理

**内核必须管理系统的运行时间以及当前的日期和时间**。

硬件为内核提供了一个**系统定时器**用以计算流逝的时间， 系统定时器以**某种频率自行触发时钟中断**，该频率可以通过编程预定， 称**节拍率**。

当时钟中断发生时， 内核就通过一种**特殊中断处理程序对其进行处理**。 内核知道连续两次时钟中断的间隔时间。 这个**间隔时间称为节拍**（tick） 。内核就是靠这种已知的时钟中断来计算墙上时间和系统运行时间。

#### **节拍率**

系统定时器频率是通过静态预处理定义的（HZ）， 在系统启动时按照 Hz 对硬件进行设置。一般 ARM 体系结构的节拍率多数都等于 100。

在编译 Linux 内核的时候可以通过图形化界面设置系统节拍率， 按照如下路径打开配置界面：
-> Kernel Features
-> Timer frequency ( [=y])
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/a68a3318144ddbd42b30282ca7e9804b.png)
Linux 内核会使用 CONFIG_HZ 来设置自己的系统时钟。

```cpp
# undef HZ
# define HZ CONFIG_HZ
# define USER_HZ 100
# define CLOCKS_PER_SEC (USER_HZ)
1234
```

宏 HZ 就是 CONFIG_HZ，HZ=100，后面编写 Linux驱动的时候会常常用到 HZ，因为 **HZ 表示一秒的节拍数，也就是频率**。

- 高节拍率：优点
  - 提高系统时间精度，如果采用 100Hz 的节拍率，时间精度就是 10ms，采用1000Hz 的话时间精度就是 1ms。能够以更高的精度运行，时间测量也更加准确。
- 高节拍率：缺点
  - 高节拍率会导致中断的产生更加频繁，频繁的中断会加剧系统的负担， 1000Hz 和 100Hz的系统节拍率相比，系统要花费 10 倍的精力去处理中断。**中断服务函数占用处理器的时间增加**，需要根据实际情况，选择合适的系统节拍率。

#### jiffies

全局变量 jiffies 用来**记录自系统启动以来产生的节拍的总数**。 启动时， 内核将该变量初始化为 0， **每次时钟中断处理程序都会增加该变量的值。**

因为一秒内时钟中断的次数等于 Hz， 所以 **jiffes 一秒内增加的值为 Hz**， 系统运行时间以秒为单位计算， 就等于**time = jiffes/Hz**( jiffes = seconds*HZ)

当 jiffies 变量的值超过它的最大存放范围后就会发生溢出， 对于 32 位无符号长整型， 最大**取值为 2^32-1**，在溢出前， 定时器节拍计数最大为 4294967295， 如果节拍数达到了**最大值后还要继续增加**的话， 它的值会**回绕到 0**。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/5bc7666e625d6b8329d56a6d6d2a8f62.png)

- 如果 unkown 超过 known 的话， time_after 函数返回真， 否则返回假。
- 如果 unkown 没有超过 known的话 time_before 函数返回真， 否则返回假。
- time_after_eq 函数在time_after上，多判断**等于**这个条件， time_before_eq 也类似

```cpp
unsigned long timeout;
timeout = jiffies + (2 * HZ); /* 超时的时间点 */
....
/* 判断有没有超时 */
if(time_before(jiffies, timeout)) 
{
    /* 超时未发生 */
} 
else 
{
    /* 超时发生 */
}
123456789101112
```

为了方便开发， Linux 内核提供了几个 jiffies 和 ms、 us、 ns 之间的转换函数
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/1b1bd1774ad7918a12a663d13fc13912.png)

- 定时 10ms
  jiffies +msecs_to_jiffies(10)
- 定时 10us
  jiffies +usecs_to_jiffies(10)

### 10.2 内核定时器

Linux 内核定时器采用系统时钟来实现，只需要提供**超时时间(定时值)\**和\**定时处理函数**即可。、

内核定时器**并不是周期性运行**的，超时以后就会自动关闭，因此如果想要**实现周期性定时**，那么就需要在**定时处理函数中重新开启定时器。**

Linux 内核使用 **timer_list 结构体**表示内核定时器

```cpp
struct timer_list
{
    struct list_head entry;
    unsigned long expires; /* 定时器超时时间， 单位是节拍数 */
    struct tvec_base *base;
    void (*function)(unsigned long); /* 定时处理函数指针 */
    unsigned long data; /* 要传递给 function 函数的参数 */
    int slack;
};
123456789
```

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/32511492017299ed884b6a688ef335d2.png)
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/14f2bad2f281d35ad691f1acf4212d84.png)

#### **例 驱动层**

```cpp
......
#define CLOSE_CMD 		(_IO(0XEF, 0x1))	/* 关闭定时器 */
#define OPEN_CMD		(_IO(0XEF, 0x2))	/* 打开定时器 */
#define SETPERIOD_CMD	(_IO(0XEF, 0x3))	/* 设置定时器周期命令 */
......
/* timer设备结构体 */
struct timer_dev{
	......
	int timeperiod; 		/* 定时周期,单位为ms */
	struct timer_list timer;/* 定义一个定时器*/
	spinlock_t lock;		/* 定义自旋锁 */
};

struct timer_dev timerdev;	/* timer设备 */


static int timer_open(struct inode *inode, struct file *filp)
{
	......
	timerdev.timeperiod = 1000;		/* 默认周期为1s */
	ret = led_init();				/* 初始化LED IO */
	if (ret < 0) {
		return ret;
	}
	return 0;
}

//ioctl函数，和应用层ioctl对应，和open，close类似
static long timer_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct timer_dev *dev =  (struct timer_dev *)filp->private_data;
	int timerperiod;
	unsigned long flags;
	
	switch (cmd) {
		case CLOSE_CMD:		/* 关闭定时器 */
			del_timer_sync(&dev->timer);
			break;
		case OPEN_CMD:		/* 打开定时器 */
			spin_lock_irqsave(&dev->lock, flags);
			timerperiod = dev->timeperiod;
			spin_unlock_irqrestore(&dev->lock, flags);
			mod_timer(&dev->timer, jiffies + msecs_to_jiffies(timerperiod));
			break;
		case SETPERIOD_CMD: /* 设置定时器周期 */
			spin_lock_irqsave(&dev->lock, flags);
			dev->timeperiod = arg;
			spin_unlock_irqrestore(&dev->lock, flags);
			mod_timer(&dev->timer, jiffies + msecs_to_jiffies(arg));
			break;
		default:
			break;
	}
	return 0;
}

/* 设备操作函数 */
static struct file_operations timer_fops = {
	.owner = THIS_MODULE,
	.open = timer_open,
	.unlocked_ioctl = timer_unlocked_ioctl,
};

/* 定时器回调函数 */
void timer_function(unsigned long arg)
{
	struct timer_dev *dev = (struct timer_dev *)arg;
	static int sta = 1;
	int timerperiod;
	unsigned long flags;

	sta = !sta;		/* 每次都取反，实现LED灯反转 */
	gpio_set_value(dev->led_gpio, sta);
	
	/* 重启定时器 */
	spin_lock_irqsave(&dev->lock, flags);
	timerperiod = dev->timeperiod;
	spin_unlock_irqrestore(&dev->lock, flags);
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->timeperiod)); 
 }

/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init timer_init(void)
{
	......
	/* 6、初始化timer，设置定时器处理函数,还未设置周期，所有不会激活定时器 */
	init_timer(&timerdev.timer);
	timerdev.timer.function = timer_function;
	timerdev.timer.data = (unsigned long)&timerdev;
	return 0;
}

static void __exit timer_exit(void)
{
	gpio_set_value(timerdev.led_gpio, 1);	/* 卸载驱动的时候关闭LED */
	del_timer_sync(&timerdev.timer);		/* 删除timer */
#if 0
	del_timer(&timerdev.tiemr);
#endif
	......
}
123456789101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899100101102103104105
```

#### **例 应用层**

```cpp
/* 命令值 */
#define CLOSE_CMD 		(_IO(0XEF, 0x1))	/* 关闭定时器 */
#define OPEN_CMD		(_IO(0XEF, 0x2))	/* 打开定时器 */
#define SETPERIOD_CMD	(_IO(0XEF, 0x3))	/* 设置定时器周期命令 */
int main(int argc, char *argv[])
{
	int fd, ret;
	char *filename;
	unsigned int cmd;
	unsigned int arg;
	unsigned char str[100];

	if (argc != 2) {
		printf("Error Usage!\r\n");
		return -1;
	}

	filename = argv[1];

	fd = open(filename, O_RDWR);
	if (fd < 0) {
		printf("Can't open file %s\r\n", filename);
		return -1;
	}

	while (1) {
		printf("Input CMD:");
		ret = scanf("%d", &cmd);
		if (ret != 1) {				/* 参数输入错误 */
			gets(str);				/* 防止卡死 */
		}

		if(cmd == 1)				/* 关闭LED灯 */
			cmd = CLOSE_CMD;
		else if(cmd == 2)			/* 打开LED灯 */
			cmd = OPEN_CMD;
		else if(cmd == 3) {
			cmd = SETPERIOD_CMD;	/* 设置周期值 */
			printf("Input Timer Period:");
			ret = scanf("%d", &arg);
			if (ret != 1) {			/* 参数输入错误 */
				gets(str);			/* 防止卡死 */
			}
		}
		ioctl(fd, cmd, arg);		/* 控制定时器的打开和关闭 */	
	}
	close(fd);
}

12345678910111213141516171819202122232425262728293031323334353637383940414243444546474849
```

## 十一、设备控制接口（ioctl）

ioctl是设备驱动程序中对设备的**I/O通道进行管理**的函数。有些命令是实在找不到对应的操作函数, 拓展一些file_operations给出的接口中没有的**自定义功能**，则需要使用到**ioctl()函数**。一些**没办法归类的函数就统一放在ioctl这个函数操作**中，通过指定的命令来实现对应的操作。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/ad2abe0316e6d6fbede43603c0bea4f5.png)

### 11.1 应用层

需要规定一些**命令码**，这些命令码在**应用程序和驱动程序中需要保持一致**。应用程序只需向驱动程序下发一条指令码，用来通知它执行哪条命令。

```cpp
#include <sys/ioctl.h>
int ioctl(int fd, unsigned long request, (...)arg);
12
```

- fd：文件描述符
- request：命令码，应用程序通过下发命令码来控制驱动程序完成对应操作。
- (…)arg：可变参数arg，一些情况下应用程序需要向驱动程序传参，参数就通过arg来传递。只能传递一个参数，但内核不会检查这个参数的类型。那么就有两种传参方式：**只传一个整数，传递一个指针**。
- 返回值：如果ioctl执行成功，它的返回值就是驱动程序中ioctl接口给的返回值，驱动程序可以通过返回值向用户程序传参。但驱动程序最好返回一个非负数，因为用户程序中的ioctl运行失败时一定会返回-1并设置全局变量errorno。

### 11.2 驱动层

驱动程序的ioctl函数体中，实现了一个**switch-case结构，每一个case对应一个命令码**，case内部是驱动程序实现该命令的相关操作。

```cpp
#include <linux/ioctl.h>
long (*unlocked_ioctl) (struct file * fp, unsigned int request, unsigned long args);
long (*compat_ioctl) (struct file * fp, unsigned int request, unsigned long args);
123
```

- inode和fp用来确定被操作的设备
- request就是用户程序下发的命令
- args就是用户程序在必要时传递的参数
- 返回值：可以在函数体中随意定义返回值，这个返回值也会被直接返回到用户程序中。通常使用非负数表示正确的返回，而返回一个负数系统会判定为ioctl调用失败。
- unlocked_ioctl在无大内核锁（BKL）的情况下调用。64位用户程序运行在64位的kernel，或32位的用户程序运行在32位的kernel上，都是调用unlocked_ioctl函数。
- compat_ioctl是64位系统提供32位ioctl的兼容方法，也在无大内核锁的情况下调用。即如果是32位的用户程序调用64位的kernel，则会调用compat_ioctl。如果驱动程序没有实现compat_ioctl，则用户程序在执行ioctl时会返回错误Not a typewriter。
- 在字符设备驱动开发中，一般情况下只要实现 unlocked_ioctl 函数即可，因为在 vfs 层的代码是直接调用 unlocked_ioctl 函数

### 12.2 ioctr应用和驱动的协议

ioctl函数的第二个参数 **cmd 为用户与驱动的协议**，**理论上可以为任意 int 型数据**，，但是为了确保该协议的唯一性，ioctl 命令应该使用更科学严谨的方法赋值，在linux中，提供了一种 ioctl 命令的统一格式，将 32 位 int 型数据划分为四个位段，如下图所示：
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/0e4a70414b5fff0142dccbc51b3b6358.png)

- dir（direction）：ioctl 命令访问模式（数据传输方向），占据 2 bit，可以为 _IOC_NONE、_IOC_READ、_IOC_WRITE、_IOC_READ | _IOC_WRITE，分别指示了四种访问模式：**无数据、读数据、写数据、读写数据**
- type（device type）：设备类型，占据 8 bit，也称为 “幻数” 或者 “魔数”，可以为任意 char 型字符，例如‘a’、’b’、’c’ 等等，其主要作用是**使 ioctl 命令有唯一的设备标识**
- nr（number）：命令**编号/序数**，占据 8 bit，可以为任意 unsigned char 型数据，取值范围 0~255，如果定义了多个 ioctl 命令，通常从 0 开始编号递增
- size：与体系结构相关，ARM下占14bit(_IOC_SIZEBITS)，如果数据是int，内核给这个赋的值就是sizeof(int)。

在内核中，提供**了宏接口以生成上述格式的 ioctl 命令**：

```cpp
#include/uapi/asm-generic/ioctl.h
 
#define _IOC(dir,type,nr,size) \
    (((dir)  << _IOC_DIRSHIFT) | \
     ((type) << _IOC_TYPESHIFT) | \
     ((nr)   << _IOC_NRSHIFT) | \
     ((size) << _IOC_SIZESHIFT))
1234567
```

**宏 _IOC() 衍生的接口来直接定义 ioctl 命令**

```cpp
#include/uapi/asm-generic/ioctl.h
 
/* used to create numbers */
#define _IO(type,nr)        _IOC(_IOC_NONE,(type),(nr),0)
#define _IOR(type,nr,size)  _IOC(_IOC_READ,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOW(type,nr,size)  _IOC(_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOWR(type,nr,size) _IOC(_IOC_READ|_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))
1234567
```

- _IO(type, nr)：用来定义不带参数的ioctl命令
- _IOR(type,nr,size)：用来定义用户程序向驱动程序写参数的ioctl命令
- _IOW(type,nr,size)：用来定义用户程序从驱动程序读参数的ioctl命令
- _IOWR(type,nr,size)：用来定义带读写参数的驱动命令

**内核还提供了反向解析 ioctl 命令的宏接口：**

```cpp
// include/uapi/asm-generic/ioctl.h
/* used to decode ioctl numbers */
#define _IOC_DIR(nr)        (((nr) >> _IOC_DIRSHIFT) & _IOC_DIRMASK)
#define _IOC_TYPE(nr)       (((nr) >> _IOC_TYPESHIFT) & _IOC_TYPEMASK)
#define _IOC_NR(nr)     (((nr) >> _IOC_NRSHIFT) & _IOC_NRMASK)
#define _IOC_SIZE(nr)       (((nr) >> _IOC_SIZESHIFT) & _IOC_SIZEMASK)
123456
```

- _IOC_DIR(nr) ：提取方向
- _IOC_TYPE(nr) ：提取幻数
- _IOC_NR(nr) ：提取序数
- _IOC_SIZE(nr) ：提取数据大小

## 十二、中断机制

中断是指 CPU 在执行程序的过程中， 出现了某些**突发事件急待处理**， CPU 必须**暂停当前程序的执行**，**转去处理突发事件**， 处理完毕后又返回原程序被中断的位置继续执行。

### 12.1 中断API函数

1. **获取中断号函数**
   每个中断都有一个中断号，通过**中断号即可区分不同的中断**。在 Linux 内核中使用一个 **int 变量表示中断号**，

   或者中断号， 中断信息一般写到了**设备树里面**， 可以通过 irq_of_parse_and_map 函数从 **interupts 属性**中提取到对应的设备号。

   ```cpp
   unsigned int irq_of_parse_and_map(struct device_node *dev,int index)
   1
   ```

   - dev： 设备节点
   - index：索引号， interrupts 属性可能包含多条中断信息，通过 index 指定要获取的信息。
   - 返回值：中断号
   - 

   **使用 GPIO 的话**，可以使用 gpio_to_irq 函数来**获取 gpio 对应的中断号**

   ```cpp
   int gpio_to_irq(unsigned int gpio)
   1
   ```

   - gpio： 要获取的 GPIO 编号
   - 返回值： GPIO 对应的中断号

2. **申请中断函数**
   Linux 内核中要想使用某个**中断是需要申请**的， request_irq 函数用于申请中断，
   request_irq函数可能会**导致睡眠**，因此**不能在中断上下文或者其他禁止睡眠的代码段中使用 request_irq 函数**。
   request_irq 函数会**激活(使能)中断**，所以**不需要手动去使能中断**。

   ```cpp
   int request_irq(unsigned int irq,
                   irq_handler_t handler,
                   unsigned long flags,
                   const char *name,
                   void *dev)
   12345
   ```

- irq：要申请中断的中断号

- handler：中断处理函数，当中断发生会执行此中断处理函数

- flags：中断标志，可以在文件 include/linux/interrupt.h 里面查看所有的中断标志

- name：中断名字，设置以后可以在/proc/interrupts 文件中看到对应的中断名字

- dev： 如果将 **flags 设置为 IRQF_SHARED**， **dev 用来区分不同的中断**，一般情况下将dev 设置为设备结构体， dev 会传递给中断处理函数 irq_handler_t 的第二个参数。

- 返回值： 0 中断申请成功，负值中断申请失败，如果返回-EBUSY 表示中断已经被申请了

  **中断标志**
  ![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/1e16768377b05177a3e936d2ee18cbb4.png)

1. **中断释放函数**
   中断使用完成以后就要通过 **free_irq 函数释放掉相应的中断**。 **如果中断不是共享的，free_irq 会删除中断处理函数并且禁止中断**。

   ```cpp
   void free_irq(unsigned int irq,void *dev)
   1
   ```

   - irq： 要释放的中断号
   - dev：如果中断设置为共享(IRQF_SHARED)，此参数用来区分具体的中断。共享中断只有在释放最后中断处理函数的时候才会被禁止掉

2. **中断处理函数**

   使用 request_irq 函数申请中断的时候需要**设置中断处理函数**

   ```cpp
   irqreturn_t (*irq_handler_t) (int, void *)
   1
   ```

   - 第一个参数：要中断处理函数要相应的中断号

   - 第二个参数：一个指向 void 的指针，是个通用指针，需要与 request_irq 函数的 dev 参数保持一致。用于区分共享中断的不同设备，dev 也可以指向设备数据结构

   - 返回值：irqreturn_t 类型

     **irqreturn_t 类型**定义如下所示：

     ```cpp
     enum irqreturn {
         IRQ_NONE = (0 << 0),
         IRQ_HANDLED = (1 << 0),
         IRQ_WAKE_THREAD = (1 << 1),
     };
     typedef enum irqreturn irqreturn_t;
     123456
     ```

     irqreturn_t 是个枚举类型， 一共有三种返回值。 一般中断服务函数返回值使用如下形式

     ```cpp
     return IRQ_RETVAL(IRQ_HANDLED)
     1
     ```

3. **中断使能和禁止函数**

   enable_irq 和 disable_irq 用于使能和禁止指定的中断。

   ```cpp
   void enable_irq(unsigned int irq)
   void disable_irq(unsigned int irq)
   void disable_irq_nosync(unsigned int irq)
   123
   ```

   - irq：要禁止的中断号
   - disable_irq 函数要等到**当前正在执行的中断处理函数执行完才返回**， 因此需要保证不会产生新的中断， 并且确保所有已经开始执行的中断处理程序已经全部退出。
   - disable_irq_nosync 函数调用以后立即返回， 不会等待当前中断处理程序执行完毕。

   **使能/关闭全局中断**

   ```cpp
   local_irq_enable()
   local_irq_disable()
   12
   ```

   - local_irq_enable 用于使能当前处理器中断系统，
   - local_irq_disable 用于禁止当前处理器中断系统。
   - 一般不能直接简单粗暴的通过这两个函数来打开或者关闭全局中断，这样会使系统崩溃。

   在打开或者关闭全局中断时，要考虑到别的任务的感受，**要保存中断状态，处理完后要将中断状态恢复到以前的状态**

   ```cpp
   local_irq_save(flags)
   local_irq_restore(flags)
   12
   ```

   - local_irq_save 函数用于禁止中断，并且将中断状态保存在 flags 中。
   - local_irq_restore 用于恢复中断，将中断到 flags 状态。

### 12.2 中断的上下部

为保证系统实时性， 中断服务程序必须足够简短，如果都在中断服务程序中完成， 则会严重降低中断的实时性，
所以， linux 系统提出了一个概念： 把**中断服务程序分为两部分： 上半部-下半部** 。主要目的就是实现**中断处理函数的快进快出**

中断服务程序分为**上半部（top half）和下半部（bottom half）**，**上半部负责读中断源，并在清中断后登记中断下半部，而耗时的工作在下半部处理。**

**上半部**只能通过**中断处理程序实现**， **下半部**的实现目前有 **3 种实现方式**， 分别为： **软中断、 tasklet 、工作队列（work queues）**

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/0c10e7507d8864bd44ce7261d15ca643.png)

#### （1）软中断

Linux 内核使用**结构体 softirq_action 表示软中断**

```cpp
struct softirq_action
{
    void (*action)(struct softirq_action *);
};
1234
```

在 kernel/softirq.c 文件中一共定义了 **10 个软中断**

```cpp
static struct softirq_action softirq_vec[NR_SOFTIRQS];
1
```

NR_SOFTIRQS 是枚举类型

```cpp
enum
{
    HI_SOFTIRQ=0, /* 高优先级软中断 */
    TIMER_SOFTIRQ, /* 定时器软中断 */
    NET_TX_SOFTIRQ, /* 网络数据发送软中断 */
    NET_RX_SOFTIRQ, /* 网络数据接收软中断 */
    BLOCK_SOFTIRQ,
    BLOCK_IOPOLL_SOFTIRQ,
    TASKLET_SOFTIRQ, /* tasklet 软中断 */
    SCHED_SOFTIRQ, /* 调度软中断 */
    HRTIMER_SOFTIRQ, /* 高精度定时器软中断 */
    RCU_SOFTIRQ, /* RCU 软中断 */
    NR_SOFTIRQS
};
1234567891011121314
```

一共有 10 个软中断，数组 softirq_vec 有 10 个元素。 **softirq_action 结构体中的 action 成员变量**就是**软中断的服务函数**。

数组 **softirq_vec 是个全局数组**，因此**所有的 CPU(对于 SMP 系统而言)都可以访问到**，**每个 CPU 都有自己的触发和控制机制**，并且只执行自己所触发的软中断。但是**各个 CPU 所执行的软中断服务函数确是相同**的，都是数组 softirq_vec 中定义的 action 函数。

- 要使用软中断，必须先使用 open_softirq 函数注册对应的软中断处理函数

  ```cpp
  void open_softirq(int nr, void (*action)(struct softirq_action *))
  //nr：要开启的软中断，也就是上面的10个软中断
  //action：软中断对应的处理函数
  123
  ```

- 注册好软中断以后需要通过 raise_softirq 函数触发

  ```cpp
  void raise_softirq(unsigned int nr)
  //nr：要触发的软中断 
  12
  ```

#### （2）tasklet

**tasklet是通过软中断实现的**， 软中断用轮询的方式处理， 假如是最后一种中断， 则必须循环完所有的中断类型， 才能最终执行对应的处理函数。

为了**提高中断处理数量，改进处理效率**， 产生了 tasklet 机制。 tasklet 采用**无差别的队列机制**， 有中断时才执行， 免去了循环查表之苦。

**tasklet 机制的优点**：

- 无类型数量限制， 效率高， 无需循环查表， 支持 SMP 机制。
- 一种特定类型的 tasklet 只能运行在一个 CPU 上， 不能并行， 只能串行执行。
- 多个不同类型的 tasklet 可以并行在多个CPU 上。
- 软中断是静态分配的， 在内核编译好之后， 就不能改变。
- 但 tasklet 就灵活许多， 可以在运行时改变，比如添加模块时 。

调用 tasklet 以后， tasklet 绑定的函数**并不会立马执行**， 而是有中断以后， 经过一个很短的不确定时间在来执行。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/2696b0bdad6b7dff571d678859e06090.png)
Linux 内核使用 **tasklet_struct 结构体**来表示 tasklet

```cpp
struct tasklet_struct
{ 
    struct tasklet_struct *next; /* 下一个 tasklet */
    unsigned long state; /* tasklet 状态 */
    atomic_t count; /* 计数器， 记录对 tasklet 的引用数 */
    void (*func)(unsigned long); /* tasklet 执行的函数 */
    unsigned long data; /* 函数 func 的参数 */
};
12345678
```

- next： 链表中的下一个 tasklet， 方便管理和设置 tasklet
- state: tasklet 的状态
- count： 表示 tasklet 是否出在激活状态， 如果是 0， 就处在激活状态， 如果非 0， 就处在非激活状态
- void (*func)(unsigned long)： 结构体中的 func 成员是 tasklet 的绑定函数， data 是它唯一的参数。func 函数就是 tasklet 要执行的处理函数，用户定义函数内容，相当于中断处理函数
- date： 函数执行的传递给func的参数

如果**要使用 tasklet**， 必须**先定义一个 tasklet**， **然后使用 tasklet_init 函数初始化 tasklet**

```cpp
void tasklet_init(struct tasklet_struct *t,
					void (*func)(unsigned long),
					unsigned long data);
123
```

- t：要初始化的 tasklet
- func： tasklet 的处理函数。
- data： 要传递给 func 函数的参数

使用**宏 DECLARE_TASKLET 一次性完成 tasklet 的定义和初始化**， DECLARE_TASKLET 定义在include/linux/interrupt.h 文件中

```cpp
DECLARE_TASKLET(name, func, data)
1
```

- name：要定义的 tasklet 名字， 就是一个 tasklet_struct 类型的变量
- func：tasklet 的处理函数
- data：传递给 func 函数的参数

在**上半部中断处理函数**中**调用 tasklet_schedule 函数**就能使 **tasklet 在合适的时间运行**

```cpp
void tasklet_schedule(struct tasklet_struct *t)
1
```

- t：要调度的 tasklet，DECLARE_TASKLET 宏里面的 name， tasklet_struct 类型的变量

**杀死 tasklet 使用 tasklet_kill 函数**,这个函数会等待 tasklet 执行完毕， 然后再将它移除。 该函数可能会引起休眠， 所以要**禁止在中断上下文中使用。**

```cpp
tasklet_kill(struct tasklet_struct *t)
1
```

- t：要删除的 tasklet

**tasklet模板**

```cpp
/* 定义 taselet */
struct tasklet_struct testtasklet;
 
/* tasklet 处理函数 */
void testtasklet_func(unsigned long data)
{
    /* tasklet 具体处理内容 */
}
 
/* 中断处理函数 */
irqreturn_t test_handler(int irq, void *dev_id)
{
    ......
    /* 调度 tasklet */
    tasklet_schedule(&testtasklet);
    ......
}
 
/* 驱动入口函数 */
static int __init xxxx_init(void)
{
    ......
    /* 初始化 tasklet */
    tasklet_init(&testtasklet, testtasklet_func, data);
    /* 注册中断处理函数 */
    request_irq(xxx_irq, test_handler, 0, "xxx", &xxx_dev);
    ......
}
12345678910111213141516171819202122232425262728
```

- ①： 定义一个 tasklet 结构体
- ②： 动态初始化 tasklet
- ③： 编写 tasklet 的执行函数
- ④： 在中断上文调用 tasklet
- ⑤： 卸载模块的时候删除 tasklet

#### （3）工作队列（workqueue）

**工作队列（workqueue）** 是实现中断下文的机制之一， 是一种将**工作推后执行的形式**。

工作队列在进程上下文执行，工作队列将**要推后的工作**交给一个**内核线程去执行**，因为**工作队列工作在进程上下文**，因此**工作队列允许睡眠或重新调度**

**工作队列tasklet 机制有什么不同呢？**
tasklet 也是实现中断下文的机制, 最主要的区别是 **tasklet不能休眠**， 而**工作队列是可以休眠**的。 所以， **tasklet 可以用来处理比较耗时间的事情**， 而**工作队列可以处理非常复杂并且更耗时间的事情**。因此如果**要推后的工作**可以睡眠就可以**选择工作队列**，否则的话就只能选择软中断或tasklet。

Linux 内核使用 **work_struct 结构体**表示一个工作

```cpp
struct work_struct {
    atomic_long_t data;
    struct list_head entry;
    work_func_t func; /* 工作队列处理函数 */
};
12345
```

这些**工作组织成工作队列**，工作队列使用 **workqueue_struct 结构体**表示

```cpp
struct workqueue_struct {
    struct list_head pwqs;
    struct list_head list;
    struct mutex mutex;
    int work_color;
    int flush_color;
    atomic_t nr_pwqs_to_flush;
    struct wq_flusher *first_flusher;
    struct list_head flusher_queue;
    struct list_head flusher_overflow;
    struct list_head maydays;
    struct worker *rescuer;
    int nr_drainers;
    int saved_max_active;
    struct workqueue_attrs *unbound_attrs;
    struct pool_workqueue *dfl_pwq;
    char name[WQ_NAME_LEN];
    struct rcu_head rcu;
    unsigned int flags ____cacheline_aligned;
    struct pool_workqueue __percpu *cpu_pwqs;
    struct pool_workqueue __rcu *numa_pwq_tbl[];
};
12345678910111213141516171819202122
```

Linux 内核使用**工作者线程(worker thread)\**来处理工作队列中的各个工作， Linux 内核使用\**worker 结构体表示工作者线程**

**每个 worker 都有一个工作队列**，**工作者线程处理自己工作队列中的所有工作**。在驱动开发中，只需要定义工作(work_struct)即可，关于工作队列和工作者线程基本不用去管。

```cpp
struct worker {
    union {
        struct list_head entry;
        struct hlist_node hentry;
    };
    struct work_struct *current_work;
    work_func_t current_func;
    struct pool_workqueue *current_pwq;
    bool desc_valid;
    struct list_head scheduled;
    struct task_struct *task;
    struct worker_pool *pool;
    struct list_head node;
    unsigned long last_active;
    unsigned int flags;
    int id;
    char desc[WORKER_DESC_LEN];
    struct workqueue_struct *rescue_wq;
};
12345678910111213141516171819
```

**初始化工作**：INIT_WORK

```cpp
#define INIT_WORK(_work, _func)
1
```

- _work ：要初始化的工作（work_struct)
- _func ：工作对应的处理函数

**工作的创建和初始化**：DECLARE_WORK

```cpp
#define DECLARE_WORK(n, f)
1
```

- n ：定义的工作(work_struct)
- f： 工作对应的处理函数

**工作的调度函数**： schedule_work

```cpp
bool schedule_work(struct work_struct *work)
1
```

- work： 要调度的工作。
- 返回值： 0 成功，其他值 失败

**工作队列模块**

```cpp
/* 定义工作(work) */
struct work_struct testwork;
 
/* work 处理函数 */
void testwork_func_t(struct work_struct *work);
{
    /* work 具体处理内容 */
}
 
/* 中断处理函数 */
irqreturn_t test_handler(int irq, void *dev_id)
{
    ......
    /* 调度 work */
    schedule_work(&testwork);
    ......
}
 
/* 驱动入口函数 */
static int __init xxxx_init(void)
{
    ......
    /* 初始化 work */
    INIT_WORK(&testwork, testwork_func_t);
 
    /* 注册中断处理函数 */
    request_irq(xxx_irq, test_handler, 0, "xxx", &xxx_dev);
    ......
}
1234567891011121314151617181920212223242526272829
```

### 12.3 设备树中的中断节点

如果一个设备**需要用到中断功能**，需要在**设备树中配置好中断属性信息**， 因为设备树是用来描述硬件信息的， 然后 Linux 内核通过设备树配置的中断属性来配置中断功能。

**例：imx6ull中断控制器节点**

```cpp
intc:interrupt-controller @00a01000
{
    compatible = "arm,cortex-a7-gic";
    #interrupt - cells = < 3>;
    interrupt - controller;
    reg = <0x00a01000 0x1000>,
    <0x00a02000 0x100>;
};
 
gpio5 : gpio @020ac000{
    compatible = "fsl,imx6ul-gpio", "fsl,imx35-gpio";
    reg = <0x020ac000 0x4000>;
    interrupts = <GIC_SPI 74 IRQ_TYPE_LEVEL_HIGH>, 
                 <GIC_SPI 75 IRQ_TYPE_LEVEL_HIGH>;
    gpio-controller;
    #gpio-cells = <2>;
    interrupt-controller;
    #interrupt-cells = <2>;
};
12345678910111213141516171819
```

**①#interrupt-cells**：此中断控制器下设备的 cells 大小，一般会使用 interrupts 属性描述中断信息， #interrupt-cells 描述了 interrupts 属性的 cells 大小， 一条信息有几个cells。 每个 cells 都是 32 位整型值， 对于 ARM 处理的 GIC 来说， 一共有 3 个 cells。

- 第一个 cells： 中断类型， 0 表示 SPI 中断， 1 表示 PPI 中断
- 第二个 cells： 中断号， SPI中断号的范围为 0~987， PPI中断号的范围为 0~15
- 第三个 cells： 标志， bit[3:0]表示中断触发类型， 为1表示上升沿触发， 为2表示下降沿触发， 为4表示高电平触发， 为8表示低电平触发。 bit[15:8]为 PPI 中断的 CPU 掩码

**②interrupt-controller** 节点为空， 表示**当前节点是中断控制器。**

**③interrupts** ：描述中断源信息， 对于 gpio5 来说一共有两条信息: 中断类型是 SPI， 触发电平是 IRQ_TYPE_LEVEL_HIGH, 中断源 一个是74， 一个是 75

**④interrupt-parent**，指定父中断，也就是中断控制器。

## 十三、阻塞与非阻塞IO

### 13.1 阻塞与非阻塞IO原理

这里的 IO 指的是 **Input/Output（输入/输出）**：是**应用程序对驱动设备的输入/输出操作**。

#### 阻塞IO

阻塞IO操作是指在执行设备操作时， 若**不能获得资源**， 则**挂起进程直到满足可操作的条件后再进行操作**。

被挂起的进程进入**睡眠状态**， 被从调度器的运行队列移走， 直到等待的条件**被满足该进程会唤醒**。

在阻塞访问时， 不能获取资源的进程将进入休眠， 它将 CPU 资源让给其他进程。 因为阻塞的进程会进入休眠状态， 所以必须确保有一个地方能够唤醒休眠的进程。**唤醒进程最大可能发生在中断函数里面**， 因为在硬件资源获得的同时往往伴随着一个中断。Linux 内核提供了等待队列(wait queue)来实现阻塞进程的唤醒工作。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/25121fe5c959edd085b72ac40e7ed51b.png)
如图，应用程序调用 read 函数从设备中读取数据，当**设备不可用**或**数据未准备好**的时候就会进入到**休眠态**。等**设备可用**的时候就会**从休眠态唤醒**，然后从设备中读取数据返回给应用程序。

```cpp
int fd;
int data = 0;
fd = open("/dev/xxx_dev", O_RDWR); /* 阻塞方式打开，默认是阻塞 */
ret = read(fd, &data, sizeof(data)); /* 读取数据 */
1234
```

#### 非阻塞IO

非阻塞IO操作是在**不能进行设备操作**时， 并不挂起， **要么放弃， 要么不停地查询**， **直至可以进行操作为止**。非阻塞的进程则不断尝试， 直到可以进行 I/O。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/d0123525cf2f159bf8cc460a10e5e17a.png)
应用程序使用**非阻塞**访问方式从**设备读取数据**，当设备不可用或数据未准备好的时候会**立即向内核返回一个错误码**，表示数据读取失败。应用程序会再次重新读取数据，一直往复循环，直到数据读取成功。

若用户以非阻塞的方式访问设备文件， 则当设备资源不可获取时， 设备驱动的 xxx_read（） 、 xxx_write（ ） 等操作应立即返回， read（ ） 、 write（ ） 等系统调用也随即被返回， 应用程序收到-EAGAIN 返回值。

```cpp
int fd;
int data = 0;
fd = open("/dev/xxx_dev", O_RDWR | O_NONBLOCK); /* 非阻塞方式打开 */
ret = read(fd, &data, sizeof(data)); /* 读取数据 */
1234
```

### 13.2 阻塞IO使用

#### 应用层（默认打开）

```cpp
fd = open(filename, O_RDWR);
if (fd < 0) 
{
	printf("Can't open file %s\r\n", filename);
    return -1;
}

ret = read(fd, &data, sizeof(data));
if (ret < 0) 
{ 
	  printf("don't know how to read\n");
	  /* 数据读取错误或者无效 */
} else 
{ 
	  /* 数据读取正确 */
	  if (data) /* 读取到数据 */
	  printf("key value = %#X\r\n", data);
}
123456789101112131415161718
```

#### 驱动层（等待队列）

在 Linux 驱动程序中，使用**等待队列（ Wait Queue）** 来实现**阻塞进程的唤醒**。

Linux 内核的**等待队列**是以**双循环链表**为基础数据结构， 与**进程调度机制紧密结合**， 能够用于实现核心的异步事件通知机制。

它有两种数据结构： **等待队列头（wait_queue_head_t）** 和**等待队列项（wait_queue_t）**。等待队列头和等待队列项中都包含一个 list_head 类型的域作为连接件。 它通过一个**双链表** 把**等待 task的头**和**等待的进程列表**链接起来。

1. **等待队列头**
   如果要在驱动中使用等待队列，必须**创建并初始化一个等待队列头**。等待队列头使用**结构体 wait_queue_head_t** 来表示

   ```cpp
   struct __wait_queue_head {
       spinlock_t lock; //自旋锁
       struct list_head task_list; //链表头
   };
   typedef struct __wait_queue_head wait_queue_head_t;
   12345
   ```

   定义好等待队列头以后需要初始化， 使用 **init_waitqueue_head 函数初始化等待队列头**。

   ```cpp
   void init_waitqueue_head(wait_queue_head_t *q)
   //q：要初始化的等待队列头
   12
   ```

2. **等待队列项**
   每个访问设备的进程都是一个队列项， 当**设备不可用时**就要将这些**进程对应的等待队列项添加到等待队列里面**。结构体 **wait_queue_t 表示等待队列项**

   ```cpp
   struct __wait_queue {
       unsigned int flags;
       void *private;
       wait_queue_func_t func;
       struct list_head task_list;
   };
   typedef struct __wait_queue wait_queue_t;
   1234567
   ```

   使用宏 **DECLARE_WAITQUEUE 定义并初始化一个等待队列项**

   ```cpp
   DECLARE_WAITQUEUE(name, tsk)
   //name：等待队列项的名字
   //tsk：等待队列项属于哪个任务(进程)， 一般设置为 current
   123
   ```

   在**内核中 current 相当于全局变量** ， 表 示 **当 前 进 程** 。所以DECLARE_WAITQUEUE是给当前**正在运行的进程创建并初始化了一个等待队列项。**

3. **添加/删除队列**
   当**设备不可访问**的时就需要将**进程对应的等待队列项** **添加**到前面创建的**等待队列头中**， 只有添加到等待队列头中以后进程才能进入休眠态。

   ```cpp
   void add_wait_queue(wait_queue_head_t *q,wait_queue_t *wait)
   //q： 等待队列项要加入的等待队列头
   //wait：要加入的等待队列项 
   123
   ```

   当**设备可以访问**后再**将进程对应的等待队列项**从**等待队列头中移除**即可。

   ```cpp
   void remove_wait_queue(wait_queue_head_t *q,wait_queue_t *wait)
   //q： 要删除的等待队列项所处的等待队列头
   //wait：要删除的等待队列项
   123
   ```

4. **唤醒等待睡眠进程**
   当**设备可以使用**的时就要**唤醒进入休眠态的进程**， 唤醒可以使用如下两个函数

   ```cpp
   void wake_up(wait_queue_head_t *q) //功能： 唤醒所有休眠进程
   void wake_up_interruptible(wait_queue_head_t *q)//功能： 唤醒可中断的休眠进程
   //q ：要唤醒的等待队列头
   123
   ```

   这两个函数会将这个**等待队列头中**的**所有进程都唤醒**。
   wake_up 函 数 可 以 唤 醒 处 于TASK_INTERRUPTIBLE 和 TASK_UNINTERRUPTIBLE 状 态 的 进 程 ，
   wake_up_interruptible 函数只能唤醒处于 TASK_INTERRUPTIBLE 状态的进程

5. **等待事件**
   除了主动唤醒以外， 也可以**设置等待队列等待某个事件**， 当这个**事件满足以后**就**自动唤醒等待队列**中的进程。
   调用的时要**确认 condition 值是真还是假**， 如果**调用 condition 为真， 则不会休眠**。

   ```cpp
   wait_event(wq, condition)
   //不可中断的阻塞等待， 让调用进程进入不可中断的睡眠状态， 
   //在等待队列里面睡眠直到 condition变成真， 被内核唤醒。
   wait_event_interruptible(queue,condition) 
   //可中断的阻塞等待， 让调用进程进入可中断的睡眠状态， 
   //直到 condition 变成真被内核唤醒或被信号打断唤醒。
   wait_event_timeout(queue,condition,timeout)
   //功能和 wait_event 类似，但是此函数可以添加超时时间，以 jiffies 为单位。 
   //如果所给的睡眠时间为负数则立即返回 。
   //如果在睡眠期间被唤醒,且condition 为真则返回剩余的睡眠时间, 
   //否则继续睡眠直到到达或超过给定的睡眠时间,然后返回 0 。
   wait_event_interruptible_timeout(queue,condition,timeout)
   //与 wait_event_timeout 函数类似， 如果在睡眠期间被信号打断则返回 ERESTARTSYS 错误码。
   12345678910111213
   ```

   使用**等待队列实现阻塞**访问重点注意两点：

   - ①、将任务或者进程加入到等待队列头，
   - ②、在合适的点唤醒等待队列，一般都是中断处理函数里面。

6. **使用模板**

   ```cpp
   /* imx6uirq 设备结构体 */
   struct imx6uirq_dev
   {
   	......
       wait_queue_head_t r_wait; /*  读等待队列头 */
   };
   
   struct imx6uirq_dev imx6uirq; /* irq 设备 */
   
   static irqreturn_t key0_handler(int irq, void *dev_id)
   {   
       struct imx6uirq_dev *dev = (struct imx6uirq_dev *)dev_id;
       tasklet_schedule(&dev->irqkeydesc[0].testtasklet);
       printk("tasklet ok\n");
       return IRQ_RETVAL(IRQ_HANDLED);
   }
   static void testtasklet_func(unsigned long data)
   {
      	......
       if(atomic_read(&dev->releasekey))  /*  完成一次按键过程 */
       {                                      
           /* wake_up(&dev->r_wait); */
           wake_up_interruptible(&dev->r_wait);
       }
   }
   
   static int keyio_init(void)
   {
     	......
       /*  初始化等待队列头 */
       init_waitqueue_head(&imx6uirq.r_wait);
       return 0;
   }
   
   static ssize_t imx6uirq_read(struct file *filp, char __user *buf,size_t cnt, loff_t *offt)
   {
    	......
   #if 0
   	/*  加入等待队列，等待被唤醒, 也就是有按键按下 */
   	ret = wait_event_interruptible(dev->r_wait, atomic_read(&dev->releasekey));			
   	if (ret) {
   		goto wait_error;
   	}
   #else
       DECLARE_WAITQUEUE(wait, current);	/* 定义一个等待队列 */
   	if(atomic_read(&dev->releasekey) == 0) /* 没有按键按下 */
       {	
   		add_wait_queue(&dev->r_wait, &wait);	/* 将等待队列添加到等待队列头 */
   		__set_current_state(TASK_INTERRUPTIBLE);/* 设置任务状态 */
   		schedule();							/* 进行一次任务切换 */
   		if(signal_pending(current))	 /* 判断是否为信号引起的唤醒 */
           {			
   			ret = -ERESTARTSYS;
   			goto wait_error;
   		}
   		__set_current_state(TASK_RUNNING);      /* 将当前任务设置为运行状态 */
   	    remove_wait_queue(&dev->r_wait, &wait);    /* 将对应的队列项从等待队列头删除 */
   	}
   #endif
       ......
       
   wait_error:
   	set_current_state(TASK_RUNNING);		/* 设置任务为运行态 */
   	remove_wait_queue(&dev->r_wait, &wait);	/* 将等待队列移除 */
   	return ret;
   }
   
   static struct file_operations imx6uirq_fops = 
   {
       .owner = THIS_MODULE,
       .open = imx6uirq_open,
       .read = imx6uirq_read,
   };
   12345678910111213141516171819202122232425262728293031323334353637383940414243444546474849505152535455565758596061626364656667686970717273
   ```

![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/206e69d59bf0c34a088823b33ad5edc4.png)

- 在设备结构体中添加一个等待队列头 r_wait，因为在 Linux 驱动中处理阻塞 IO需要用到等待队列。
- 调用 init_waitqueue_head 函数初始化等待队列头 r_wait。
- read驱动函数手动休眠等待按键按下
- 法一：
  - 采用等待事件来处理 read 的阻塞访问，wait_event_interruptible 函数等待 releasekey 有效，也就是有按键按下。
  - 如果按键没有按下的话进程就会进入休眠状态，因为采用了 wait_event_interruptible 函数，因此进入休眠态的进程可以被信号打断。
- 法二：
  - 首先使用 DECLARE_WAITQUEUE 宏定义一个等待队列，
  - 如果没有按键按下的话就使用 add_wait_queue 函数将当前任务的等待队列添加到等待队列头 r_wait 中。
  - 随后调用__set_current_state 函数设置当前进程的状态为 TASK_INTERRUPTIBLE，也就是可以被信号打断。
  - 接下来调用 schedule 函数进行一次任务切换，当前进程就会进入到休眠态。
  - 如果有按键按下，那么进入休眠态的进程就会唤醒，然后接着从休眠点开始运行。首先通过 signal_pending 函数判断一下进程是不是由信号唤醒的，如果是由信号唤醒的话就直接返回-ERESTARTSYS 这个错误码。
  - 如果不是由信号唤醒的(也就是被按键唤醒的)那么就在 调用__set_current_state 函数将任务状态设置为TASK_RUNNING，然后在调用 remove_wait_queue 函数将进程从等待队列中删除。
- 定时器中断处理函数执行，表示有按键按下，先在判断一下是否是一次有效的按键，如果是的话就通过 wake_up 或者 wake_up_interruptible 函数来唤醒等待队列r_wait。
- 完成read函数后，设置任务为运行态，将等待队列移除

### 13.3 非阻塞IO使用（轮询）

#### 应用层

如果用户**应用程序**以**非阻塞的方式**访问设备，设备驱动程序就要提供非阻塞的处理方式，即**轮询**。 **poll、 epoll 和 select 可以用于处理轮询。**

**应用程序**通过 **select、 epoll 或 poll 函数**来**查询设备是否可以操作**，如果可以操作的话就从设备读取或者向设备写入数据。当**应用程序调用 select、 epoll 或 poll 函数**的时，**设备驱动程序**中的 **poll 函数就会执行**，因此需要在设备**驱动程序中编写 poll 函数**。

##### **1.select**

```cpp
int select(int nfds,
            fd_set *readfds,
            fd_set *writefds,
            fd_set *exceptfds,
            struct timeval *timeout)
12345
```

- **nfds**： 所要监视的这三类文件描述集合， 最大文件描述符加 1

- readfds、 writefds 和 exceptfds

  ：指向描述符集合。指明了关心哪些描述符.这三个参数都是 fd_set 类型的， fd_set 类型变量的每一个位都代表了一个文件描述符。

  - readfds 用于监视指定描述符集的**读变化**，监视这些文件是否可以读取，只要这些集合里面有一个文件可以读取， seclect 就会返回一个大于 0 的值表示文件可以读取。如果没有文件可以读取，会根据 timeout 参数来判断是否超时。若将 readfs设置为 NULL，表示不关心任何文件的读变化。
  - writefs 用于监视文件是否可以**进行写操作**。
  - exceptfds 用于**监视文件的异常**。

- **timeout**:超时时间，当调用 select 函数等待某些文件描述符可以设置超时时间

- **返回值**： **0: 超时发生**，没有文件描述符可以进行操作； **-1: 发生错误**； **其他值**: **可以进行操作的文件描述符个数**

**从一个设备文件中读取数据**，要**定义一个 fd_set 变量**，这个变量要**传递给参数 readfds**。当定义好一个 fd_set 变量以后可以使用如下所示几个宏进行操作：

```cpp
void FD_ZERO(fd_set *set)  
void FD_SET(int fd, fd_set *set)
void FD_CLR(int fd, fd_set *set)
int FD_ISSET(int fd, fd_set *set)
1234
```

- FD_ZERO：将 fd_set 变量的所有位都清零
- FD_SET：将 fd_set 变量的某个位置 1，向 fd_set 添加fd文件描述符
- FD_CLR：将 fd_set变量的某个位清零，将fd文件描述符从 fd_set 中删除
- FD_ISSET: 测试文件描述符 fd是否属于某个集合

**超时时间使用结构体 timeval 表示**， 当 timeout 为 **NULL 的时候就表示无限期的等待**。

```cpp
struct timeval {
    long tv_sec; /* 秒 */
    long tv_usec; /* 微妙 */
};
1234
```

**应用层select函数非阻塞访问模块**

```cpp
void main(void)
{
    int ret, fd; /* 要监视的文件描述符 */
    fd_set readfds; /* 读操作文件描述符集 */
    struct timeval timeout; /* 超时结构体 */
    fd = open("dev_xxx", O_RDWR | O_NONBLOCK); /* 非阻塞式访问 */
 
    FD_ZERO(&readfds); /* 清除 readfds */
    FD_SET(fd, &readfds); /* 将 fd 添加到 readfds 里面 */
 
    /* 构造超时时间 */
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000; /* 500ms */
 
    ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
    switch (ret) 
    {
        case 0: /* 超时 */
            printf("timeout!\r\n");
            break;
        case -1: /* 错误 */
            printf("error!\r\n");
            break;
        default: /* 可以读取数据 */
            if(FD_ISSET(fd, &readfds)) /* 判断fd 文件描述符是否在readfds里面 */
            { 
                /* 使用 read 函数读取数据 */
            }
            break;
    }
}
12345678910111213141516171819202122232425262728293031
```

##### **2.poll**

在单个线程中， **select** 函数能够**监视的文件描述符数量有最大的限制**，一般为 **1024**。可以修改内核将监视的文件描述符数量改大。这时可以使用 **poll 函数**，poll 函数本质上和 select 没有太大的差别，但是 poll 函数**没有最大文件描述符限制。**

```cpp
int poll(struct pollfd *fds,
        nfds_t nfds,
        int timeout)    
123
```

- fds： 要监视的文件描述符集合以及要监视的事件, 为一个数组，**数组元素都是结构体 pollfd类型的**。
- nfds： poll 函数要监视的文件描述符数量
- timeout： 超时时间，单位为 ms
- 返回值：返回 revents 域中不为 0 的 pollfd 结构体个数，发生事件或错误的文件描述符数量； 0：超时； -1：发生错误，并且设置 errno 为错误类型

**pollfd 结构体**

```cpp
struct pollfd {
    int fd; /* 文件描述符 */
    short events; /* 请求的事件 */
    short revents; /* 返回的事件 */
};
12345
```

- fd 是要监视的文件描述符，如果 fd 无效，则events 监视事件也无效，并且 revents返回 0。
- events 是要监视的事件，可监视的事件类型如下：
  - POLLIN 有数据可以读取。
  - POLLPRI 有紧急的数据需要读取。
  - POLLOUT 可以写数据。
  - POLLERR 指定的文件描述符发生错误。
  - POLLHUP 指定的文件描述符挂起。
  - POLLNVAL 无效的请求。
  - POLLRDNORM 等同于 POLLIN
- revents 是返回的事件， 由 Linux 内核设置具体的返回事件。

**应用层 poll 函数非阻塞访问模块**

```cpp
void main(void)
{
    int ret;
    int fd; /* 要监视的文件描述符 */
    struct pollfd fds;
 
    fd = open(filename, O_RDWR | O_NONBLOCK); /* 非阻塞式访问 */
 
    /* 构造结构体 */
    fds.fd = fd;
    fds.events = POLLIN; /* 监视数据是否可以读取 */
    ret = poll(&fds, 1, 500); /* 轮询文件是否可操作，超时 500ms */
    if (ret) /* 数据有效 */
    { 
        ......
        /* 读取数据 */
        ......
    } 
    else if (ret == 0) /* 超时 */
    { 
        ......
    } 
    else if (ret < 0) /* 错误 */
    { 
        ......
    }
}
123456789101112131415161718192021222324252627
```

##### **3.epoll**

**selcet 和 poll 函数**都会随着所监听的 **fd 数量的增加**，出现**效率低下的问题**，而且poll 函数每次**必须遍历所有的描述符来检查就绪的描述符**，这个过程很浪费时间。

**epoll 就是为处理大并发而准备的**，一般常常在**网络编程中**使用 epoll 函数。

**应用程序需要先使用 epoll_create 函数创建一个 epoll 句柄**

```cpp
int epoll_create(int size)
1
```

- size：从 Linux2.6.8 开始此参数已经没有意义了，填写一个大于 0 的值就可以
- 返回值： epoll 句柄，如果为-1 的话表示创建失败

epoll 句柄创建成功以后使用 **epoll_ctl 函数**向其中**添加要监视的文件描述符以及监视的事件**

```cpp
int epoll_ctl(int epfd,
            int op,
            int fd,
            struct epoll_event *event)
1234
```

- epfd： 要操作的 epoll 句柄，使用 epoll_create 函数创建的 epoll 句柄
- op： 要对epoll 句柄进行的操作，可以设置为：
  - EPOLL_CTL_ADD 向 epfd 添加文件参数 fd 表示的描述符
  - EPOLL_CTL_MOD 修改参数 fd 的 event 事件。
  - EPOLL_CTL_DEL 从 epfd 中删除 fd 描述符
- fd：要监视的文件描述符
- event： 要监视的事件类型，为 epoll_event 结构体类型指针
- 返回值： 0：成功； -1：失败，并且设置 errno 的值为相应的错误码。

**监视的事件**类型为 **epoll_event 结构体**类型指针

```cpp
struct epoll_event {
        uint32_t events; /* epoll 事件 */
        epoll_data_t data; /* 用户数据 */
};
1234
```

- events 表示要监视的事件，可选的事件如下
  - EPOLLIN 有数据可以读取
  - EPOLLOUT 可以写数据
  - EPOLLPRI 有紧急的数据需要读取
  - EPOLLERR 指定的文件描述符发生错误
  - EPOLLHUP 指定的文件描述符挂起
  - EPOLLET 设置 epoll 为边沿触发，默认触发模式为水平触发
  - EPOLLONESHOT 一次性的监视，当监视完成以后还需要再次监视某个 fd，就需要将fd 重新添加到 epoll 里面

最后通过 **epoll_wait 函数来等待事件的发生**

```cpp
int epoll_wait(int epfd,
    struct epoll_event *events,
    int maxevents,
    int timeout)
1234
```

- epfd： 要等待的 epoll
- events： 指向epoll_event结构体的数组，当有事件发生的时Linux内核会填写 events，调用者可以根据 events 判断发生了哪些事件
- maxevents： events 数组大小，必须大于 0
- timeout： 超时时间，单位为 ms
- 返回值： 0：超时； -1：错误；其他值：准备就绪的文件描述符数量

epoll 更多的是用在**大规模的并发服务器上**，因为在这种场合下 select 和 poll 并不适合。当设计到的文件描述符比较少的时候就适合用 selcet 和 poll。

#### 驱动层

当应用程序调用 **select 或 poll 函数**来对**驱动程序进行非阻塞访问**，**驱动程序**file_operations 操作集中的 **poll 函数**就会执行。

```cpp
unsigned int (*poll) (struct file *filp, struct poll_table_struct *wait)
1
```

- filp： 要打开的设备文件(文件描述符)
- wait： poll_table_struct 类型指针， 由应用程序传递进来的，将此参数传递给poll_wait()
- 返回值: 向应用程序返回设备或者资源状态，返回状态有：
  - POLLIN 有数据可以读取。
  - POLLPRI 有紧急的数据需要读取。
  - POLLOUT 可以写数据。
  - POLLERR 指定的文件描述符发生错误。
  - POLLHUP 指定的文件描述符挂起。
  - POLLNVAL 无效的请求。
  - POLLRDNORM 等同于 POLLIN

需要在**驱动程序的 poll 函数**中**调用 poll_wait 函数**， poll_wait 函数**不会引起阻塞**，只是将**应用程序添加到 poll_table 中**

```cpp
void poll_wait(struct file * filp, wait_queue_head_t * wait_address, poll_table *p)
1
```

- filp： 要打开的设备文件(文件描述符)
- wait_address：要添加到 poll_table 中的等待队列头
- p：file_operations 中 poll 函数的 wait 参数

**驱动层poll函数模板（和应用层select、poll对应）**

```cpp
/* imx6uirq 设备结构体 */
struct imx6uirq_dev
{
  	......
    wait_queue_head_t r_wait; /*  读等待队列头 */
};

struct imx6uirq_dev imx6uirq; /* irq 设备 */

static irqreturn_t key0_handler(int irq, void *dev_id)
{
    
    struct imx6uirq_dev *dev = (struct imx6uirq_dev *)dev_id;
    tasklet_schedule(&dev->irqkeydesc[0].testtasklet);
    printk("tasklet ok\n");
    return IRQ_RETVAL(IRQ_HANDLED);
}
static void testtasklet_func(unsigned long data)
{
    ......
    if(atomic_read(&dev->releasekey))  /*  完成一次按键过程 */
    {                                      
        /* wake_up(&dev->r_wait); */
        wake_up_interruptible(&dev->r_wait);
    }
}

static int keyio_init(void)
{
   	......
    /*  初始化等待队列头 */
    init_waitqueue_head(&imx6uirq.r_wait);
    return 0;
}

static ssize_t imx6uirq_read(struct file *filp, char __user *buf,size_t cnt, loff_t *offt)
{
    int ret = 0;
    unsigned char keyvalue = 0;
    unsigned char releasekey = 0;
    struct imx6uirq_dev *dev = (struct imx6uirq_dev *)filp->private_data;


   if (filp->f_flags & O_NONBLOCK)	/* 非阻塞访问 */
   { 
		if(atomic_read(&dev->releasekey) == 0)	/* 没有按键按下，返回-EAGAIN */
			return -EAGAIN;
	} 
	else /* 阻塞访问 */
    {							
		/* 加入等待队列，等待被唤醒,也就是有按键按下 */
 		ret = wait_event_interruptible(dev->r_wait, atomic_read(&dev->releasekey)); 
		if (ret) 
        {
			goto wait_error;
		}
    }
	......
wait_error:
	return ret;
	......
}

unsigned int imx6uirq_poll(struct file *filp, struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	struct imx6uirq_dev *dev = (struct imx6uirq_dev *)filp->private_data;

	poll_wait(filp, &dev->r_wait, wait);	/* 将等待队列头添加到poll_table中 */
	
	if(atomic_read(&dev->releasekey)) {		/* 按键按下 */
		mask = POLLIN | POLLRDNORM;			/* 返回PLLIN */
	}
	return mask;
}

static struct file_operations imx6uirq_fops = 
{
    .owner = THIS_MODULE,
    .open = imx6uirq_open,
    .read = imx6uirq_read,
    .poll = imx6uirq_poll,
};
1234567891011121314151617181920212223242526272829303132333435363738394041424344454647484950515253545556575859606162636465666768697071727374757677787980818283
```

## 十四、异步通知机制（信号通知）

阻塞IO和非阻塞IO都需要应用程序**主动去查询设备的使用情况**。Linux 提供了**异步通知机制**，**驱动程序能主动向应用程序发出通知**。

**信号是在软件层次上对中断的一种模拟**，驱动可以通过主动向应用程序发送信号的方式通知可以访问，应用程序获取到信号后就可以从驱动设备中读取或者写入数据。

### 14.1 异步通知应用程序

1. **注册信号处理函数**

   应用程序根据驱动程序所使用的信号来设置信号的处理函数，应用程序使用 **signal 函数来设置信号的处理函数**。

   ```cpp
   sighandler_t signal(int signum, sighandler_t handler)
   1
   ```

   - signum：要设置处理函数的信号
   - handler： 信号的处理函数
   - 返回值： 设置成功返回信号的前一个处理函数，设置失败的话返回 SIG_ERR。

   **信号中断处理函数**

   ```cpp
   typedef void (*sighandler_t)(int)
   1
   ```

   在处理函数执行相应的操作即可。

2. **将本应用程序的进程号告诉给内核**

   **fcntl系统调用**可以用来对**已打开的文件描述符**进行各种**控制操作以改变已打开文件的的各种属性**

   ```cpp
   #include <unistd.h>
   #include <fcntl.h>
   int fcntl(int fd, int cmd);
   int fcntl(int fd, int cmd, long arg);
   int fcntl(int fd, int cmd, struct flock *lock);
   12345
   ```

   使用 **fcntl(fd, F_SETOWN, getpid())\**将本应用程序的\**进程号告诉给内核**。
   ![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/55ff51aec9e9492f0a92e3cce43d04af.png)

3. **开启异步通知**

   主要是通过 **fcntl 函数**设置进程状态为 FASYNC，经过这一步，**驱动程序中的 fasync 函数就会执行**。

   ```cpp
   flags = fcntl(fd, F_GETFL); /* 获取当前的进程状态 */
   fcntl(fd, F_SETFL, flags | FASYNC); /* 开启当前进程异步通知功能 */
   12
   ```

4. **应用程序模板**

   ```cpp
   static int fd = 0;	/* 文件描述符 */
   
   static void sigio_signal_func(int signum)
   {
   	int err = 0;
   	unsigned int keyvalue = 0;
   
   	err = read(fd, &keyvalue, sizeof(keyvalue));
   	if(err < 0) {
   		/* 读取错误 */
   	} else {
   		printf("sigio signal! key value=%d\r\n", keyvalue);
   	}
   }
   
   int main(int argc, char *argv[])
   {
   	int flags = 0;
   	char *filename;
   
   	if (argc != 2) {
   		printf("Error Usage!\r\n");
   		return -1;
   	}
   
   	filename = argv[1];
   	fd = open(filename, O_RDWR);
   	if (fd < 0) {
   		printf("Can't open file %s\r\n", filename);
   		return -1;
   	}
   
   	/* 设置信号SIGIO的处理函数 */
   	signal(SIGIO, sigio_signal_func);
   	
   	fcntl(fd, F_SETOWN, getpid());		/* 设置当前进程接收SIGIO信号 	*/
   	flags = fcntl(fd, F_GETFL);			/* 获取当前的进程状态 			*/
   	fcntl(fd, F_SETFL, flags | FASYNC);	/* 设置进程启用异步通知功能 	*/	
   
   	while(1) {
   		sleep(2);
   	}
   
   	close(fd);
   	return 0;
   }
   12345678910111213141516171819202122232425262728293031323334353637383940414243444546
   ```

### 14.2 异步通知驱动程序

1. 内核要使用异步通知需要在驱动程序中定义一个 **fasync_struct 结构体**的**指针变量**

   一般将 fasync_struct 结构体指针变量**定义到设备结构体**中即可。

   ```cpp
   struct fasync_struct {
       spinlock_t fa_lock;
       int magic;
       int fa_fd;
       struct fasync_struct *fa_next;
       struct file *fa_file;
       struct rcu_head fa_rcu;
   };
   struct fasync_struct *async_queue; /* 异步相关结构体 */
   123456789
   ```

2. 然后需要在**设备驱动**中实现 file_operations 操作集中的 **fasync 函数**。

   ```cpp
   int (*fasync) (int fd, struct file *filp, int on)
   1
   ```

3. fasync 函数里面一般通过**调用 fasync_helper 函数**来**初始化前面定义的 fasync_struct 结构体指针**

   ```cpp
   int fasync_helper(int fd, struct file * filp, int on, struct fasync_struct **fapp)
   1
   ```

   fasync_helper 函数的**前三个参数**就是 **fasync 函数的三个参数**，**第四个参数**就是**要初始化的 fasync_struct 结构体指针变量。**

   当**应用程序**通过**fcntl(fd, F_SETFL, flags | FASYNC)改变fasync 标记**的时，驱动程序file_operations 操作集中的 **fasync 函数**就会执行。

4. 当设备可以访问的时候，驱动程序需要向应用程序发出信号，相当于产生中断。 **kill_fasync函数负责发送指定的信号**

   ```cpp
   void kill_fasync(struct fasync_struct **fp, int sig, int band)
   1
   ```

   - fp：要操作的 fasync_struct
   - sig： 要发送的信号
   - band： 可读时设置为 POLL_IN，可写时设置为 POLL_OUT

5. 最后，在关闭驱动文件的时候需要在 file_operations 操作集中的 **release 函数中释放 fasync_struct**，fasync_struct 的释放函数为 fasync_helper。

   ```cpp
   xxx_fasync(-1, filp, 0); /* 删除异步通知 */
   1
   ```

   xxx_fasync 函数就是file_operations **操作集中的 fasync 函数**。

6. 驱动程序模板

   ```cpp
   /* imx6uirq设备结构体 */
   struct imx6uirq_dev{
   	......
   	struct fasync_struct *async_queue;		/* 异步相关结构体 */
   };
   
   struct imx6uirq_dev imx6uirq;	/* irq设备 */
   
   void timer_function(unsigned long arg)
   {
   	......
   	if(atomic_read(&dev->releasekey)) {		/* 一次完整的按键过程 */
   		if(dev->async_queue)
   			kill_fasync(&dev->async_queue, SIGIO, POLL_IN);	/* 释放SIGIO信号 */
   	}
   	......
   }
   /*
    * @description     : fasync函数，用于处理异步通知
    * @param - fd		: 文件描述符
    * @param - filp    : 要打开的设备文件(文件描述符)
    * @param - on      : 模式
    * @return          : 负数表示函数执行失败
    */
   static int imx6uirq_fasync(int fd, struct file *filp, int on)
   
   {
   	struct imx6uirq_dev *dev = (struct imx6uirq_dev *)filp->private_data;
   	return fasync_helper(fd, filp, on, &dev->async_queue);  /* 初始化fasync_struct 结构体指针*/
   }
   
   static int imx6uirq_release(struct inode *inode, struct file *filp)
   {
   	return imx6uirq_fasync(-1, filp, 0); /* 删除异步通知 */
   
   }
   
   /* 设备操作函数 */
   static struct file_operations imx6uirq_fops = {
   	......
   	.poll = imx6uirq_poll,
   	.fasync = imx6uirq_fasync,
   	.release = imx6uirq_release,
   };
   1234567891011121314151617181920212223242526272829303132333435363738394041424344
   ```

## 十五、Platform设备驱动框架

### 15.1 设备驱动的分层思想

Linux 内核完全由 C 语言和汇编语言写成， 但是却频繁用到了**面向对象的设计思想**。

在**设备驱动方面**，为**同类的设备设计了一个框架**， **框架中的核心层则实现了该设备通用的一些功能**。同样的， 如果具体的设备不想使用核心层的函数， 它可以**重载**之。

```cpp
return_type core_funca(xxx_device * bottom_dev, param1_type param1, param1_type param2)
{
    if(bottom_dev->funca)
        return bottom_dev->funca(param1, param2);
    
    /* 核心层通用的 funca 代码 */
  
    bottom_dev->funca_ops1(); 	/*通用的步骤代码 A */
    ...
    bottom_dev->funca_ops2();	/*通用的步骤代码 B */   
    ...
    bottom_dev->funca_ops3();	/*通用的步骤代码 C */
}
12345678910111213
```

上述 **core_funca 的实现中**， 会**检查**底层设备**是否重载了 funca()**， 如果重载了， 就调用底层的代码， 否则直接使用通用层的。 这样做的好处是， 核心层的代码可以处理绝大多数该类设备的funca()对应的功能，只有少数特殊设备需要重新实现 funca()。

### 15.2 驱动的分离与分层

将**设备信息从设备驱动中剥离开来**，驱动使用标准方法去获取到设备信息 (比如从设备树中获取到设备信息)，根据获取到的设备信息来初始化设备。

**驱动只负责驱动，设备只负责设备，总线法将两者进行匹配**。

这就是 Linux 中的**总线(bus)、驱动(driver)和设备(device)模型**，即**驱动分离**。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/df184e0b433b4227d45aabb92cdb0186.png)
当向**系统注册一个驱动**的时，**总线会在设备中查找与之匹配的设备**，如果有，就将两者联系起来。同样的，当向系统中**注册一个设备**的时候，**总线就会在驱动中查找与之匹配的驱动**，如果有，也联系起来。 Linux 内核中大量的驱动程序都采用总线、驱动和设备模式。

### 15.3 Platform 平台总线驱动模型

在 Linux 2.6 以后的设备驱动模型中， 需关心**总线、 设备和驱动**这 3 个实体， **总线将设备和驱动绑定**。

Linux 设备和驱动通常都需要**挂接在一种总线上**， 对于本身依附于 PCI、 USB、 I2C、 SPI 等的设备而言， 这自然不是问题。但是在嵌入式系统里面， 在 SoC 系统中集成的独立外设控制器、 挂接在 SoC内存空间的外设等却不依附于此类总线。

基于这一背景， Linux 发明了一种**虚拟的总线**， **称为 platform 总线**， 相应的设备称为 platform_device， 驱动称为 platform_driver。 平台总线模型就是把原来的驱动C文件给分成了俩个 C 文件，**一个是 device.c（描述硬件信息，设备树可替代）， 一个是 driver.c（驱动信息）** 。
![在这里插入图片描述](https://i-blog.csdnimg.cn/blog_migrate/dea8c23df9b90f532ce6e900827abd12.png)

#### 一、platform 驱动

在 Linux 内核中， 用**platform_driver结构体**表示platform驱动，platform_driver 结构体**定义指定名称**的**平台设备驱动注册函数**和**平台设备驱动注销函数**

```cpp
struct platform_driver {
    int (*probe)(struct platform_device *);
    int (*remove)(struct platform_device *);
    void (*shutdown)(struct platform_device *);
    int (*suspend)(struct platform_device *, pm_message_t state);
    int (*resume)(struct platform_device *);
    struct device_driver driver;
    const struct platform_device_id *id_table;
    bool prevent_deferred_probe;
};
12345678910
```

- probe 函数: 当驱动与设备**匹配成功以后 probe 函数就会执行**, 一般驱动的提供者会编写
- remove函数：当 driver 和 device 任意一个 **remove 的时， 就会执行该函数**
- driver： device_driver 结构体变量， Linux 内核里面大量使用到了面向对象的思维， **device_driver相当于基类**，提供了最基础的驱动框架。 **plaform_driver 继承了这个基类，在此基础上又添加了一些特有的成员变量**
- **id_table 表保存 id 信息**。 这些 id 信息**存放着platformd 驱动所支持的驱动类型**。 id_table是个表(数组)， 每个元素的类型为 platform_device_id，

**platform_device_id 结构体**内容如下：

```cpp
struct platform_device_id {
    char name[PLATFORM_NAME_SIZE];
    kernel_ulong_t driver_data;
};
1234
```

**device_driver 结构体**定义内容如下：

```cpp
struct device_driver {
    const char *name;
    struct bus_type *bus;
 
    struct module *owner;
    const char *mod_name; /* used for built-in modules */
 
    bool suppress_bind_attrs; /* disables bind/unbind via sysfs */
 
    const struct of_device_id *of_match_table; //设备树的驱动匹配表
    const struct acpi_device_id *acpi_match_table;
    int (*probe) (struct device *dev);
    int (*remove) (struct device *dev);
    void (*shutdown) (struct device *dev);
    int (*suspend) (struct device *dev, pm_message_t state);
    int (*resume) (struct device *dev);
    const struct attribute_group **groups;
    const struct dev_pm_ops *pm;
    struct driver_private *p;
};
1234567891011121314151617181920
```

**of_match_table表**就是采用**设备树**时**驱动使用的匹配表**，也是数组，每个匹配项都为 **of_device_id 结构体**类型

```cpp
struct of_device_id {
    char name[32];
    char type[32];
    char compatible[128];
    const void *data;
};
123456
```

**compatible:** 在支持设备树的内核中， 就是通过**设备节点的compatible属性值**和**of_match_table中每个项目的 compatible 成员变量**进行比较， 如果有**相等的**就表示**设备和此驱动匹配成功**。

**驱动和设备匹配成功后**，**驱动会从设备里面获得硬件资源**， 匹配成功了后， driver.c 要从 device.c（或者是设备树） 中获得硬件资源， 那么 driver.c 就是在 probe 函数中获得的。

#### 二、platform 设备（可以被设备树替代）

在 platform 平台下用**platform_device结构体表示platform设备**， 如果内核**支持设备树的话就不用使用 platform_device 来描述设备**， 使用设备树去描述platform_device即可。

```cpp
struct platform_device {
    const char *name;
    int id;
    bool id_auto;
    struct device dev;
    u32 num_resources;
    struct resource *resource;
 
    const struct platform_device_id *id_entry;
    char *driver_override; /* Driver name to force a match */
 
    /* MFD cell pointer */
    struct mfd_cell *mfd_cell;
 
    /* arch specific additions */
    struct pdev_archdata archdata;
};
1234567891011121314151617
```

- name：设备名字，要和所使用的 platform 驱动的 name 字段相同，否则设备就无法匹配到对应的驱动。比如对应的 platform 驱动的name字段为xxx-gpio，则此name字段也要设置为xxx-gpio。
- id ：用来区分如果设备名字相同的时，通过在后面添加一个数字来代表不同的设备
- dev：内置的device结构体
- num_resources ：资源数量，一般为resource 资源的大小(个数)，ARRAY_SIZE 来测量一个数组的元素个数。
- resource：指向一个资源结构体数组，即设备信息，比如外设寄存器等。

Linux 内核使用 **resource结构体**表示资源

```cpp
struct resource {
    resource_size_t start;
    resource_size_t end;
    const char *name;
    unsigned long flags;
    struct resource *parent, *sibling, *child;
};
1234567
```

**start 和 end** 分别表示**资源的起始和终止信息**，对于内存类的资源，表示内存起始和终止地址， name 表示资源名字， flags 表示资源类型

使用 **platform_device_register 函数**将**设备信息注册到 Linux 内核中**

```cpp
int platform_device_register(struct platform_device *pdev)
1
```

- pdev：要注册的 platform 设备
- 返回值： 负数，失败； 0，成功

如果**不再使用 platform**可以通过 **platform_device_unregister 函数注销掉相应的 platform设备**

```cpp
void platform_device_unregister(struct platform_device *pdev)
1
```

- pdev：要注销的 platform 设备
- 返回值： 无

#### 三、platform 总线

platform设备和platform驱动，相当于把设备和驱动分离了， **需要 platform 总线进行配**， platform 设备和 platform 驱动进行内核注册时， 都是注册到总线上。

在 Linux 内核中使用 **bus_type 结构体**表示总线

```cpp
struct bus_type {
    const char *name; /* 总线名字 */
    const char *dev_name;
    struct device *dev_root;
    struct device_attribute *dev_attrs;
    const struct attribute_group **bus_groups; /* 总线属性 */
    const struct attribute_group **dev_groups; /* 设备属性 */
    const struct attribute_group **drv_groups; /* 驱动属性 */
 
    int (*match)(struct device *dev, struct device_driver *drv);
    int (*uevent)(struct device *dev, struct kobj_uevent_env *env);
    int (*probe)(struct device *dev);
    int (*remove)(struct device *dev);
    void (*shutdown)(struct device *dev);
 
    int (*online)(struct device *dev);
    int (*offline)(struct device *dev);
    int (*suspend)(struct device *dev, pm_message_t state);
    int (*resume)(struct device *dev);
    const struct dev_pm_ops *pm;
    const struct iommu_ops *iommu_ops;
    struct subsys_private *p;
    struct lock_class_key lock_key;
};
123456789101112131415161718192021222324
```

**match 函数**：完成设备和驱动之间匹配的，总线使用 match 函数来**根据注册的设备来查找对应的驱动**，或者根据**注册的驱动来查找相应的设备**，因此每一条总线都必须实现此函数。

match 函数有**两个参数**： **dev 和 drv**，这两个参数分别为 device 和 device_driver 类型，即**设备和驱动**。

**platform 总线是 bus_type 的一个具体实例**

```cpp
struct bus_type platform_bus_type = {
    .name = "platform",
    .dev_groups = platform_dev_groups,
    .match = platform_match,
    .uevent = platform_uevent,
    .pm = &platform_dev_pm_ops,
};
1234567
```

**platform_match 匹配函数**， 用来匹配注册到 platform 总线的设备和驱动。

#### 四、platform 总线具体匹配方法

查看platform_match函数，**如何匹配驱动和设备的**

```cpp
static int platform_match(struct device *dev,struct device_driver *drv)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct platform_driver *pdrv = to_platform_driver(drv);
 
    /*When driver_override is set,only bind to the matching driver*/
    if (pdev->driver_override)
        return !strcmp(pdev->driver_override, drv->name);
 
    /* Attempt an OF style match first */
    if (of_driver_match_device(dev, drv))
        return 1;
 
    /* Then try ACPI style match */
    if (acpi_driver_match_device(dev, drv))
        return 1;
    /* Then try to match against the id table */
    if (pdrv->id_table)
        return platform_match_id(pdrv->id_table, pdev) != NULL;
 
    /* fall-back to driver name match */
    return (strcmp(pdev->name, drv->name) == 0);
}
1234567891011121314151617181920212223
```

**驱动和设备的匹配有四种方法**。

1. **OF 类型的匹配**
   设备树采用的匹配方式，**of_driver_match_device 函数**定义在文件 include/linux/of_device.h 中。

   device_driver 结构体(**设备驱动**)中有个名为**of_match_table的成员变量**，此成员变量**保存着驱动的compatible匹配表**，

   **设备树中**的每个**设备节点的 compatible 属性**会和 **of_match_table 表中的所有成员比较**，查看是否有**相同的条目**，如果有的话就表示**设备和此驱动匹配**，设备和驱动**匹配成功以后 probe 函数就会执行**。

2. **ACPI 匹配方式**

3. **id_table 匹配**
   每个 platform_driver 结构体(**设备驱动**)有一个 **id_table成员变量**，保存了很多 id 信息。这些 **id 信息存放着这个 platformd 驱动所支持的驱动类型**

4. **名字匹配**
   如果第三种匹配方式的 **id_table 不存在**的话就**直接比较驱动和设备的 name 字段**，如果**相等的话就匹配成功**。

对于支持设备树的 Linux 版本号，一般设备驱动为了兼容性都支持设备树和无设备树两种匹配方式。即**第一种匹配方式一般都会存在**，第三种和第四种只要存在一种就可以，一般**用的最多的还是第四种，直接比较驱动和设备的 name 字段**。

### 15.4 platform 总线框架例程

#### 一、应用层

```cpp
int main(int argc, char *argv[])
{
    int fd, retvalue;
    char *filename;
    unsigned char databuf[1];
    unsigned char readbuf[1];

    if(argc != 3)
    {
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];

    fd = open(filename, O_RDWR);
    if(fd < 0)
    {
        printf("file %s open failed!\r\n", argv[1]);
        return -1;
    }
    databuf[0] = atoi(argv[2]); /* 要执行的操作：打开或关闭 */
    if(databuf[0] ==  2)
    {
         retvalue = read(fd,readbuf,sizeof(readbuf));
        if(retvalue < 0)
        {
            printf("read file %s failed!\r\n",filename);
        }
        else
        {
            printf("read date: %x\r\n",readbuf[0]);
        }
    }else
    {
         /* 向/dev/led 文件写入数据 */
        retvalue = write(fd, databuf, sizeof(databuf));
        if(retvalue < 0)
        {
            printf("LED Control Failed!\r\n");
            close(fd);
            return -1;
        }
    }
   
    retvalue = close(fd);
    if(retvalue < 0)
    {
        printf("file %s close failed!\r\n", argv[1]);
        return -1;
    }
    return 0;
}   
1234567891011121314151617181920212223242526272829303132333435363738394041424344454647484950515253
```

#### 二、驱动层（无设备树）

##### device.c

```cpp
/* 
 * 寄存器地址定义
 */
#define CCM_CCGR1_BASE				(0X020C406C)	
#define SW_MUX_GPIO1_IO03_BASE		(0X020E0068)
#define SW_PAD_GPIO1_IO03_BASE		(0X020E02F4)
#define GPIO1_DR_BASE				(0X0209C000)
#define GPIO1_GDIR_BASE				(0X0209C004)
#define REGISTER_LENGTH				4
/*  
 * 设备资源信息，也就是LED0所使用的所有寄存器
 */
static struct resource led_resources[] = {
	[0] = {
		.start 	= CCM_CCGR1_BASE,
		.end 	= (CCM_CCGR1_BASE + REGISTER_LENGTH - 1),
		.flags 	= IORESOURCE_MEM,
	},	
	[1] = {
		.start	= SW_MUX_GPIO1_IO03_BASE,
		.end	= (SW_MUX_GPIO1_IO03_BASE + REGISTER_LENGTH - 1),
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start	= SW_PAD_GPIO1_IO03_BASE,
		.end	= (SW_PAD_GPIO1_IO03_BASE + REGISTER_LENGTH - 1),
		.flags	= IORESOURCE_MEM,
	},
	[3] = {
		.start	= GPIO1_DR_BASE,
		.end	= (GPIO1_DR_BASE + REGISTER_LENGTH - 1),
		.flags	= IORESOURCE_MEM,
	},
	[4] = {
		.start	= GPIO1_GDIR_BASE,
		.end	= (GPIO1_GDIR_BASE + REGISTER_LENGTH - 1),
		.flags	= IORESOURCE_MEM,
	},
};
/*
 * platform设备结构体 
 */
static struct platform_device leddevice = {
	.name = "imx6ul-led",
	.id = -1,
	.dev = {
		.release = &led_release,
	},
	.num_resources = ARRAY_SIZE(led_resources),
	.resource = led_resources,
};
static int __init leddevice_init(void)
{
	return platform_device_register(&leddevice);
}
static void __exit leddevice_exit(void)
{
	platform_device_unregister(&leddevice);
}
module_init(leddevice_init);
module_exit(leddevice_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("songwei");
123456789101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263
```

##### driver.c

```cpp
#define LEDDEV_CNT		1			/* 设备号长度 	*/
#define LEDDEV_NAME		"platled"	/* 设备名字 	*/
#define LEDOFF 			0
#define LEDON 			1
/* 寄存器名 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;
/* leddev设备结构体 */
struct leddev_dev{
	......	
};
struct leddev_dev leddev; 	/* led设备 */

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
    .read = led_read,
	.open = led_open,
	.write = led_write,
};

/*
 * @description		: flatform驱动的probe函数，当驱动与
 * 					  设备匹配以后此函数就会执行
 * @param - dev 	: platform设备
 * @return 			: 0，成功;其他负值,失败
 */
static int led_probe(struct platform_device *dev)
{	
	int i = 0;
	int ressize[5];
	u32 val = 0;
	struct resource *ledsource[5];

	printk("led driver and device has matched!\r\n");
	/* 1、获取资源 */
	for (i = 0; i < 5; i++) {
		ledsource[i] = platform_get_resource(dev, IORESOURCE_MEM, i); /* 依次MEM类型资源 */
		if (!ledsource[i]) {
			dev_err(&dev->dev, "No MEM resource for always on\n");
			return -ENXIO;
		}
		ressize[i] = resource_size(ledsource[i]);	
	}	
	......
}
static int led_remove(struct platform_device *dev)
{
	iounmap(IMX6U_CCM_CCGR1);
	iounmap(SW_MUX_GPIO1_IO03);
	iounmap(SW_PAD_GPIO1_IO03);
	iounmap(GPIO1_DR);
	iounmap(GPIO1_GDIR);
	......
}
/* platform驱动结构体 */
static struct platform_driver led_driver = {
	.driver		= {
		.name	= "imx6ul-led",			/* 驱动名字，用于和设备匹配 */
	},
	.probe		= led_probe,
	.remove		= led_remove,
};
static int __init leddriver_init(void)
{
	return platform_driver_register(&led_driver);
}
static void __exit leddriver_exit(void)
{
	platform_driver_unregister(&led_driver);
}
module_init(leddriver_init);
module_exit(leddriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("songwei");
1234567891011121314151617181920212223242526272829303132333435363738394041424344454647484950515253545556575859606162636465666768697071727374757677
```

#### 二、驱动层（有设备树）

##### driver.c

```cpp
#define LEDDEV_CNT		1			/* 设备号长度 	*/
#define LEDDEV_NAME		"dts_platled"	/* 设备名字 	*/
#define LEDOFF 			0
#define LEDON 			1

/* leddev设备结构体 */
struct leddev_dev{
	......
    struct device_node *node;	/* LED设备节点 */
	int led0;					/* LED灯GPIO标号 */	
};

struct leddev_dev leddev; 	/* led设备 */

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
    .read = led_read,
	.open = led_open,
	.write = led_write,
};
/*
 * @description		: flatform驱动的probe函数，当驱动与
 * 					  设备匹配以后此函数就会执行
 * @param - dev 	: platform设备
 * @return 			: 0，成功;其他负值,失败
 */
static int led_probe(struct platform_device *dev)
{	
	......
    /* 6、初始化IO */	
	leddev.node = of_find_node_by_path("/gpioled");
	if (leddev.node == NULL){
		printk("gpioled node nost find!\r\n");
		return -EINVAL;
	} 
	leddev.led0 = of_get_named_gpio(leddev.node, "led-gpio", 0);
	if (leddev.led0 < 0) {
		printk("can't get led-gpio\r\n");
		return -EINVAL;
	}
	gpio_request(leddev.led0, "led0");
	gpio_direction_output(leddev.led0, 1); /* led0 IO设置为输出，默认高电平	*/
	return 0;
}

/*
 * @description		: platform驱动的remove函数，移除platform驱动的时候此函数会执行
 * @param - dev 	: platform设备
 * @return 			: 0，成功;其他负值,失败
 */
static int led_remove(struct platform_device *dev)
{
    gpio_set_value(leddev.led0, 1); 	/* 卸载驱动的时候关闭LED */
	gpio_free(leddev.led0);				/* 释放IO 			*/

	......
}

/* 匹配列表 */
static const struct of_device_id led_of_match[] = {
	{ .compatible = "songwei-gpioled" },
	{ /* Sentinel */ }
};

/* platform驱动结构体 */
static struct platform_driver led_driver = {
	.driver		= {
		.name	= "imx6ul-led",			/* 驱动名字，用于和设备匹配 */
        .of_match_table	= led_of_match, /* 设备树匹配表 		 */
	},
	.probe		= led_probe,
	.remove		= led_remove,
};

static int __init leddriver_init(void)
{
	return platform_driver_register(&led_driver);
}

static void __exit leddriver_exit(void)
{
	platform_driver_unregister(&led_driver);
}
module_init(leddriver_init);
module_exit(leddriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("songwei");
123456789101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081828384858687
```

[![img](https://profile-avatar.csdnimg.cn/17fbf92142f84400a2fc2343d908c4cb_qq_44814825.jpg!1)拉依达的嵌入式小屋](https://blog.csdn.net/qq_44814825)

[关注](javascript:;)

- ![img](https://csdnimg.cn/release/blogv2/dist/pc/img/toolbar/like.png)412
- ![img](https://csdnimg.cn/release/blogv2/dist/pc/img/toolbar/unlike.png)
- [![img](https://csdnimg.cn/release/blogv2/dist/pc/img/toolbar/collect.png)1911](javascript:;)
- [![img](https://csdnimg.cn/release/blogv2/dist/pc/img/toolbar/comment.png)22](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#commentBox)
- [![打赏](https://csdnimg.cn/release/blogv2/dist/pc/img/toolbar/reward.png)打赏](javascript:;)
- ![img](https://csdnimg.cn/release/blogv2/dist/pc/img/toolbar/more.png)

专栏目录

[*linux**驱动**开发*简要*流程*](https://blog.csdn.net/godloveyuxu/article/details/77460361)

[Charles' home](https://blog.csdn.net/godloveyuxu)

![img](https://csdnimg.cn/release/blogv2/dist/pc/img/readCountWhite.png) 675

[在这里，以字符设备为例，分析其*驱动*程序的框架：一、编写*驱动*程序的初始化函数*驱动*程序的初始化在函数XXX_init（）中完成，包括对硬件初始化、中断函数、向内核注册等。a) 首先要理解硬件结构，搞清楚其功能、接口寄存器以及CPU怎么访问控制这些寄存器等。b) 其次要明白如何把该设备*驱动*注册到内核中。设备*驱动*程序可以直接编进内核（在移植内核时，就将该*驱动*程序编译进内核），在系统启动的时候初始化，也可以...](https://blog.csdn.net/godloveyuxu/article/details/77460361)



[*Linux**驱动**开发*(速记版)--*驱动*基础最新发布](https://blog.csdn.net/qq_42190402/article/details/142367194)

[荒野](https://blog.csdn.net/qq_42190402)

![img](https://csdnimg.cn/release/blogv2/dist/pc/img/readCountWhite.png) 1463

[讯为*开发*板，RK3568](https://blog.csdn.net/qq_42190402/article/details/142367194)

[*linux**驱动**开发**流程*](https://blog.csdn.net/EN_wang/article/details/7221240)

![img](https://csdnimg.cn/release/blogv2/dist/pc/img/readCountWhite.png) 1万+

[嵌入式*linux**驱动**开发**流程* 嵌入式系统中，操作系统是通过各种*驱动*程序来驾驭硬件设备的。设备*驱动*程序是操作系统内核和硬件设备之间的接口，它为应用程序屏蔽了硬件的细节，这样在应用程序看来，硬件设备只是一个设备文件，可以像操作普通文件一样对硬件设备进行操作。设备*驱动*程序是内核的一部分，完成以下功能： ◇ *驱动*程序的注册和注销。 ◇ 设备的打开和释放。 ◇ 设备的读写操作。 ◇ 设备的控制操作](https://blog.csdn.net/EN_wang/article/details/7221240)

[6.设备*驱动*程序的*开发*过程](https://blog.csdn.net/u010291330/article/details/45675333)

[u010291330的博客](https://blog.csdn.net/u010291330)

![img](https://csdnimg.cn/release/blogv2/dist/pc/img/readCountWhite.png) 1373

[由于嵌入式设备由于硬件种类非常丰富，在默认的内核发布版中不一定包括所有*驱动*程序。所以进行嵌入式*Linux*系统的*开发*，很大的工作量是为各种设备编写 *驱动*程序。除非系统不使用操作系统，程序直接操纵硬件。嵌入式*Linux*系统*驱动*程序*开发*与普通*Linux**开发*没有区别。可以在硬件生产厂家或者 Internet上寻找*驱动*程序，也可以根据相近的硬件*驱动*程序来改写，这样可以加快*开发*速度。实现一个嵌入式*Linux*设](https://blog.csdn.net/u010291330/article/details/45675333)

[*Linux**驱动**开发**流程*介绍](https://blog.csdn.net/gaoyanli1972/article/details/103182267)

[gaoyanli1972的博客](https://blog.csdn.net/gaoyanli1972)

![img](https://csdnimg.cn/release/blogv2/dist/pc/img/readCountWhite.png) 605

[一，如何学习*Linux**驱动* 我们学习*驱动*的目的是自己编写*驱动*。由此延伸出的学习技巧就是： 1，写一个*驱动*，尽量多参考别人的*驱动*，在他人*驱动*的基础上进行修改。尽量回避从零开始写一个*驱动*。 2，不要总是想着分析内核代码，能正确使用内核提供的相关函数即可。*Linux*内核中涉及的知识点总是互相交错，不适合初学者阅读。在有一定的基础之后，分析和自己直接相关的内核源代码，可以加深对*驱动*的理解。 3，百度是最好...](https://blog.csdn.net/gaoyanli1972/article/details/103182267)

[*Linux**驱动**开发*学习的一些必要步骤](https://blog.csdn.net/itpublisher/article/details/2431908)

[红颜无暇电脑网,最新的专业 IT 资讯-http://it.hywxfashion.cn](https://blog.csdn.net/itpublisher)

![img](https://csdnimg.cn/release/blogv2/dist/pc/img/readCountWhite.png) 455

[ 

[《*Linux*设备*驱动**开发*详解(第3版)》（即《*Linux*设备*驱动**开发*详解：基于最新的*Linux* 4.0内核》）前言热门推荐](https://blog.csdn.net/21cnbao/article/details/45322629)

[宋宝华](https://blog.csdn.net/21cnbao)

![img](https://csdnimg.cn/release/blogv2/dist/pc/img/readCountWhite.png) 4万+

[*Linux*从未停歇脚步。Linus Torvalds，世界上最伟大的程序员之一，*Linux*内核的创始人，Git的缔造者，仍然在没日没夜的合并补丁，升级内核。做技术，从来没有终南捷径，拼的就是坐冷板凳的傻劲。 这是一个连阅读都被碎片化的时代，在这样一个时代，人们趋向于激进、浮躁。内心的不安宁使我们极难静下心来研究什么。我见过许许多多的*Linux*工程师，他们的简历...](https://blog.csdn.net/21cnbao/article/details/45322629)

[*Linux**驱动**开发*基础](https://blog.csdn.net/qq_53144843/article/details/123412850)

[风间琉璃的博客](https://blog.csdn.net/qq_53144843)

![img](https://csdnimg.cn/release/blogv2/dist/pc/img/readCountWhite.png) 2万+

[一、内核态和用户态 内核态与用户态是操作系统的两种运行级别，cpu提供Ring0-Ring3三种级别的运行模式。Ring0级别最高，Ring3最低。 CPU是在两种不同的模式下运行的:Kernel Mode（内核态），在内核模式下（执行内核空间的代码），具有ring0保护级别，代码具有对硬件的所有控制权限。可以执行所有CPU指令，可以访问任意地址的内存。内核模式是为操作系统最底层，最可信的函数服务的。在内核模式下的任何异常都是灾难性的，将会导致整台机器停机。 User Mode（用户态），在用户模式下](https://blog.csdn.net/qq_53144843/article/details/123412850)

[八、*Linux*——*驱动*认知](https://blog.csdn.net/i_saic/article/details/130836707)

[i_saic的博客](https://blog.csdn.net/i_saic)

![img](https://csdnimg.cn/release/blogv2/dist/pc/img/readCountWhite.png) 447

[*驱动*就是对底层硬件设备的操作进行封装，并向上层提供函数接口。字符设备、块设备、网络设备。**字符设备：指只能一个字节一个字节读写的设备，不能随机读取设备内存中的某一数据，读取数据需要按照先后顺序。**字符设备是面向流的设备，常见的字符设备有鼠标、键盘、串口、控制台和LED设备等，字符设备*驱动*程序通常至少要实现open、close、read和write的系统调用，字符终端（/dev/console）和串口（/dev/ttyS0以及类似设备）就是两个字符设备，它们能很好的说明“流”这种抽象概念。块设备。](https://blog.csdn.net/i_saic/article/details/130836707)

[![img](https://profile-avatar.csdnimg.cn/17fbf92142f84400a2fc2343d908c4cb_qq_44814825.jpg!1)](https://blog.csdn.net/qq_44814825)

[拉依达的嵌入式小屋](https://blog.csdn.net/qq_44814825)

码龄6年[![img](https://csdnimg.cn/identity/nocErtification.png) 暂无认证](https://i.csdn.net/#/uc/profile?utm_source=14998968)







- 27万+

  访问

- [![img](https://csdnimg.cn/identity/blog5.png)](https://blog.csdn.net/blogdevteam/article/details/103478461)

  等级

- 3618

  积分

- 1万+

  粉丝

- 2237

  获赞

- 48

  评论

- 4942

  收藏

[私信](https://im.csdn.net/chat/qq_44814825)

关注

### 目录

1. [Linux驱动开发详细解析](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t0)
2. [一、驱动概念](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t1)
3. [二、驱动分类](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t2)
4. [三、驱动程序的功能](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t3)
5. [四、驱动开发前提知识](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t4)
6. 1. [4.1 内核态和用户态](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t5)
   2. [4.2 Linux下应用程序调用驱动程序流程](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t6)
   3. 1. [大致流程](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t7)
   4. [4.3 内核模块](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t8)
   5. 1. [内核模块组成](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t9)
      2. [模块操作命令](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t10)
   6. [4.4 设备号](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t11)
   7. [4.5 地址映射](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t12)
   8. 1. [MMU(Memory Manage Unit)内存管理单元](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t13)
      2. [内存映射函数](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t14)
      3. [I/O内存访问函数](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t15)
7. [五、设备树](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t16)
8. 1. [5.1 DTS、DTB和DTC](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t17)
   2. [5.2 设备树框架](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t18)
   3. [5.3 DTS语法](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t19)
   4. 1. [dtsi头文件](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t20)
      2. [设备节点](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t21)
      3. [属性](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t22)
   5. [5.4 OF操作函数](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t23)
   6. 1. [查找节点](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t24)
      2. [获取属性值](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t25)
      3. [of 函数在 led_init() 中应用](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t26)
9. [六、字符设备驱动](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t27)
10. 1. [6.1 字符设备基本驱动框架](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t28)
    2. 1. [1.模块加载](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t29)
       2. [2.注册字符设备驱动](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t30)
       3. [3.内存映射](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t31)
       4. [4.应用层和内核层传递数据](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t32)
       5. [5. 字符设备最基本框架](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t33)
       6. [6. 创建驱动节点文件](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t34)
    3. [6.2 新字符设备基本驱动框架](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t35)
    4. 1. [1.设备文件系统](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t36)
       2. [2.申请设备号](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t37)
       3. [3.注册字符设备](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t38)
       4. [4.自动创建设备节点](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t39)
       5. [5.文件私有数据](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t40)
       6. [6.新字符设备驱动程序框架](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t41)
11. [七、pinctrl子系统](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t42)
12. 1. [7.1 pinctrl 子系统主要工作内容：](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t43)
    2. [7.2 pinctrl的设备树设置](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t44)
    3. [7.3 设备树中添加pinctrl模板](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t45)
13. [八、GPIO子系统](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t46)
14. 1. [8.1 GPIO子系统工作内容](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t47)
    2. [8.2 GPIO子系统设备树设置](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t48)
    3. [8.3 API函数](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t49)
    4. [8.4 GPIO相关OF函数](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t50)
    5. [8.5 pinctrl和gpio子系统使用程序框架](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t51)
15. [九、内核并发与竞争](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t52)
16. 1. [9.1 并发与竞争概念](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t53)
    2. [9.2 原子操作](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t54)
    3. [9.2 自旋锁](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t55)
    4. [9.3 信号量](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t56)
    5. [9.4 互斥体](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t57)
17. [十、内核定时器](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t58)
18. 1. [10.1 内核时间管理](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t59)
    2. 1. [节拍率](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t60)
       2. [jiffies](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t61)
    3. [10.2 内核定时器](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t62)
    4. 1. [例 驱动层](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t63)
       2. [例 应用层](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t64)
19. [十一、设备控制接口（ioctl）](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t65)
20. 1. [11.1 应用层](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t66)
    2. [11.2 驱动层](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t67)
    3. [12.2 ioctr应用和驱动的协议](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t68)
21. [十二、中断机制](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t69)
22. 1. [12.1 中断API函数](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t70)
    2. [12.2 中断的上下部](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t71)
    3. 1. [（1）软中断](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t72)
       2. [（2）tasklet](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t73)
       3. [（3）工作队列（workqueue）](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t74)
    4. [12.3 设备树中的中断节点](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t75)
23. [十三、阻塞与非阻塞IO](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t76)
24. 1. [13.1 阻塞与非阻塞IO原理](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t77)
    2. 1. [阻塞IO](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t78)
       2. [非阻塞IO](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t79)
    3. [13.2 阻塞IO使用](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t80)
    4. 1. [应用层（默认打开）](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t81)
       2. [驱动层（等待队列）](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t82)
    5. [13.3 非阻塞IO使用（轮询）](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t83)
    6. 1. [应用层](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t84)
       2. [驱动层](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t85)
25. [十四、异步通知机制（信号通知）](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t86)
26. 1. [14.1 异步通知应用程序](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t87)
    2. [14.2 异步通知驱动程序](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t88)
27. [十五、Platform设备驱动框架](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t89)
28. 1. [15.1 设备驱动的分层思想](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t90)
    2. [15.2 驱动的分离与分层](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t91)
    3. [15.3 Platform 平台总线驱动模型](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t92)
    4. 1. [一、platform 驱动](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t93)
       2. [二、platform 设备（可以被设备树替代）](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t94)
       3. [三、platform 总线](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t95)
       4. [四、platform 总线具体匹配方法](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t96)
    5. [15.4 platform 总线框架例程](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t97)
    6. 1. [一、应用层](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t98)
       2. [二、驱动层（无设备树）](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t99)
       3. [二、驱动层（有设备树）](https://blog.csdn.net/qq_44814825/article/details/129107911?ops_request_misc=%7B%22request%5Fid%22%3A%22E755734A-83BB-4270-B4B7-2A93AC35ADFC%22%2C%22scm%22%3A%2220140713.130102334..%22%7D&request_id=E755734A-83BB-4270-B4B7-2A93AC35ADFC&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-129107911-null-null.142^v100^pc_search_result_base4&utm_term=linux 驱动&spm=1018.2226.3001.4187#t100)