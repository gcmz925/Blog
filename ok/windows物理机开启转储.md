## windows物理机开启强制转储

> CrashControl.reg

```bash
Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\i8042prt\Parameters]
"CrashOnCtrlScroll"=dword:00000001

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\kbdhid\Parameters]
"CrashOnCtrlScroll"=dword:00000001

```

### 1.写入注册表项目

创建上述 `CrashControl.reg` 文件，管理员运行，写入注册表



### 2.开启widnwos转储

系统属性→高级→启动和故障恢复→写入调试信息→完全内存转储。

### 3.重启

上述配置重启后生效。

### 4.测试

重启后，连续按两次Ctrl+Scroll Lock触发蓝屏并写入dump，是右侧的Ctrl。

（右侧Ctrl按住不动，按两下 Scroll Lock）



### 5.详细配置

> 微软官方说明

[通过键盘强制系统崩溃](https://learn.microsoft.com/zh-cn/windows-hardware/drivers/debugger/forcing-a-system-crash-from-the-keyboard)