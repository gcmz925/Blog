麒麟linux安装报错,错误显示

glibc库等级过低

报错：

```
ImportError: /opt/XXXXXXX/libstdc++.so.6: symbol __cxa_thread_atexit_impl, version GLIBC_2.18 not defined in file libc.so.6 with 
```

查看当前glibc等级:

```
[root@localhost opt]# strings /lib64/libc.so.6 | grep ^GLIBC
GLIBC_2.17
GLIBC_2.18
GLIBC_PRIVATE
```

对应的rpm包下载地址:

1.apt软件源
http://archive.kylinos.cn/kylin/KYLIN-ALL/

2.升级对应的gblic包
https://update.cs2c.com.cn/NS/V10/V10SP1/os/adv/lic/base/aarch64/Packages/

```
glibc-2.28-36.1.ky10.aarch64.rpm                   24-Aug-2020 10:28      3M
glibc-all-langpacks-2.28-36.1.ky10.aarch64.rpm     24-Aug-2020 10:28     27M
glibc-benchtests-2.28-36.1.ky10.aarch64.rpm        24-Aug-2020 10:28    462K
glibc-common-2.28-36.1.ky10.aarch64.rpm            24-Aug-2020 10:28     26M
glibc-debugutils-2.28-36.1.ky10.aarch64.rpm        24-Aug-2020 10:28      8M
glibc-devel-2.28-36.1.ky10.aarch64.rpm             24-Aug-2020 10:28      3M
glibc-help-2.28-36.1.ky10.noarch.rpm               24-Aug-2020 10:28    103K
glibc-locale-source-2.28-36.1.ky10.aarch64.rpm     24-Aug-2020 10:28      4M
glibc-nss-devel-2.28-36.1.ky10.aarch64.rpm         24-Aug-2020 10:28     11K
```

下载 `glibc-2.28-36.1.ky10.aarch64.rpm` 升级即可



后续遇到

```
ImportError: libcrypto.so.1.1: cannot open shared object file: No such file or directory
```



查看.so文件的版本信息

```
readelf -d filename.so | grep “SONAME”
```



进入单用户模式

[麒麟系统进入单用户模式](https://blog.csdn.net/weixin_43173670/article/details/138598070?utm_medium=distribute.pc_relevant.none-task-blog-2~default~baidujs_baidulandingword~default-1-138598070-blog-124593304.235^v43^pc_blog_bottom_relevance_base2&spm=1001.2101.3001.4242.2&utm_relevant_index=4)

```
找到linux开头的那一行，在末尾输入init=/bin/bash console=tty0

另外，如果这行中出现了ro，需要将其改为rw
```



修改selinux属性

```
vi /etc/selinux/config
```



修改为静态ip

```
sudo nano /etc/sysconfig/network-scripts/ifcfg-eth0
```

```
TYPE="Ethernet"
BOOTPROTO="static"
NAME="eth0"
DEVICE="eth0"
ONBOOT="yes"
IPADDR="192.168.1.100"
PREFIX="24"
GATEWAY="192.168.1.1"
DNS1="8.8.8.8"
DNS2="8.8.4.4"
```

```
sudo systemctl restart network
```



确认kdump是否已经开启

```
sudo systemctl status kdump
```

检查dump配置,会包含,crashkernel=256M

```
cat /proc/cmdline
```

强制触发转储

```
echo c > /proc/sysrq-trigger
```



查看so版本信息

```
readelf -d /usr/lib64/libselinux.so.1 | grep SONAME
```



ss -ano 限制空格个数

```
ss -ano | grep 22 | tr -s ' '
```



这是什么系统?

```
5.4.18-53-generic
```

```
cat /etc/os-release
lsb_release -a
cat /etc/issue

```



```
echo c > /proc/sysrq-trigger 
```





列出当前所有的环境变量

```
export -p //列出当前的环境变量值
```



定义环境变量赋值

```
export MYENV=7 //定义环境变量并赋值
```



添加环境变量：默认保存在~/.bash_profile，只会当次生效，要想一直生效得手动写入系统环境变量/etc/profile中

```
export  KUBECONFIG="/etc/kubernetes/admin.conf"    //添加环境变量
```



env 命令即可查看到设置后的环境变量　　



删除：但是只对此次操作有效，退出后重新连接依然存在。

```
export  -n KUBECONFIG="/etc/kubernetes/admin.conf"
```



unset 命令删除：也是暂时的，只会在当前环境有效

```
unset KUBECONFIG
```



彻底删除：

```
export添加的环境变量，默认保存在~/.bash_profile，除非手动指定添加才会保存到PATH的/etc/profile当中。

找到~/.bash_profile中要删除的那一行手动将变量删除即可

然后，source .bash_profile 使生效
```



源码下载地址

```
https://mirrors.edge.kernel.org/pub/linux/kernel/
```



有几个函数需要比较下

```
https://elixir.bootlin.com/linux/v5.0/source/arch/arm64/mm/pageattr.c
```



```

change_page_range
apply_to_page_range
```





修改内存属性

```
https://blog.csdn.net/weixin_47763623/article/details/129287591
https://elixir.bootlin.com/linux/v4.19.279/source/arch/arm64/mm/pageattr.c#L44
https://gist.github.com/ohnx/02c181c2674ca6db88ddd12b36e6c1bf#file-ttgl-c  //github
```



linux 驱动内存申请

https://blog.csdn.net/lpwsw/article/details/122168707