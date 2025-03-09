## crash + windbg

```
rpm -ivh kernel-debuginfo-common-x86_64-3.10.0-862.el7.x86_64.rpm
rpm -ivh kernel-debuginfo-3.10.0-862.el7.x86_64.rpm
```

```
crash /usr/lib/debug/lib/modules/3.10.0-1160.el7.x86_64/vmlinux /root/127.0.0.1-2023-08-22-12\:31\:24/vmcore
```

```
echo 1 > /proc/sys/kernel/sysrq
echo c > /proc/sysrq-trigger
```



分离进程

```
.detach
qd
```



windbg 日志输出等级

```
eb nt!Kd_DEFAULT_Mask 0
eb nt!Kd_DEFAULT_Mask 0xf
```

进程

```
!process 0 0
## dump 所有
!process 0 1f  
!process <>

!process 0 0 antiransom_agent.exe 
!process 0 0 <xxx>

!process ffffde06b43d04c0 1f
!object  ffffde06b43d04c0

##正在运行的,查死锁卡死问题
!running it 
!thread <>

## 加载pdb时查看出问题的
!sym noisy
!sym quiet

## 可以查看一个线程的调用栈
dsp

~* k
```

```
1、!process 0 0 目标进程名 获取目标进程EPROCESS基本信息
2、.process /p +EPROCESS信息 切换到目标进程空间
3、.reload /f /user 强制重新加载用户态符号
4、.process /i /p 目标进程的EPROCESS 侵入式调试
5、bp 目标API 执行下断点命令

.thread /p + THREAD信息
```

```
!apc
    !apc proc Process
    !apc thre Thread
    !apc KAPC
```

