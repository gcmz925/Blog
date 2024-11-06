# 虚拟机开机时出现Press ESC in 5 seconds to skip startup.nsh, any other key to continue问题的解决办法

## 问题复现：

1.问题在qemu关闭arm虚拟机后通过命令重启出现

## 解决方案：

### 1.使用UEFI Shell手动设置启动设备

1. **进入UEFI Shell**：
   - 通常在启动时按`ESC`或`F12`等键可以选择进入UEFI Shell，具体按键取决于硬件。
2. **查看文件系统**：
   - 在UEFI Shell提示符下，输入`ls`列出文件系统，或`map -r`重新扫描设备并列出文件系统标识符，如`fs0:`、`fs1:`等。
3. **进入文件系统目录**：
   - 输入`fs0:`（或其他标识符，如`fs1:`）进入对应的文件系统。
   - 使用`ls`命令查看当前目录内容。
4. **编辑启动文件`startup.nsh`**：
   - 进入正确的文件系统后，输入`edit startup.nsh`以编辑启动文件。
   - 在编辑器中，输入需要的启动文件路径，如`\EFI\ubuntu\grubx64.efi`。
   - 按`Ctrl + S`保存文件，然后按`Enter`确认。
   - 按`Ctrl + Q`退出编辑器，然后按`Enter`确认。
5. **重启系统**：
   - 输入`reset`命令重新启动系统。



### 2.使用UEFI Shell查找启动文件路径

1. **进入UEFI Shell**：启动计算机并进入UEFI Shell。方法因设备不同而异，通常是在启动时按`ESC`、`F12`、`F2`等键。

2. **列出文件系统**：

   - 在Shell提示符下，输入`map -r`命令列出所有可用的文件系统。输出结果会显示类似于`fs0:`、`fs1:`等标识符，以及它们对应的设备路径。

   - 例子：

     ```
     Shell> map -r
     ```

     输出：

     ```
     css复制代码fs0  : Acpi(HID,...)PCI(...)Sata(...)HD(...)
     fs1  : Acpi(HID,...)PCI(...)USB(...)
     ```

3. **查看文件系统内容**：

   - 输入文件系统标识符（如`fs0:`）并使用`ls`命令查看其内容，寻找`EFI`文件夹。

   - 例子：

     ```
     Shell> fs0:
     FS0> ls
     ```

     输出：

     ```
     Directory of: fs0:\
     11/02/2022  09:45 AM    <DIR>          EFI
     ```

4. **进入`EFI`目录并查找子目录**：

   - 进入`EFI`目录后，再次使用`ls`命令查看其内容。通常情况下，您会看到一个或多个子目录，代表已安装的操作系统或引导程序。

   - 例子：

     ```
     FS0> cd EFI
     FS0:\EFI> ls
     ```

     输出：

     ```
     Directory of: fs0:\EFI\
     11/02/2022  09:45 AM    <DIR>          ubuntu
     ```

5. **进入操作系统或引导程序的目录**：

   - 进入相应的目录（如`ubuntu`）并查看其中的文件。您应该能够找到类似`grubx64.efi`的文件。

   - 例子：

     ```
     FS0:\EFI> cd ubuntu
     FS0:\EFI\ubuntu> ls
     ```

     输出：

     ```
     Directory of: fs0:\EFI\ubuntu\
     11/02/2022  09:45 AM               12345 grubx64.efi
     ```

6. **记录完整路径**：

   - 找到`grubx64.efi`文件后，记录完整路径。例如，如果文件位于`fs0:`文件系统的`EFI\ubuntu`目录下，则完整路径为`fs0:\EFI\ubuntu\grubx64.efi`。在`startup.nsh`文件中，可以使用`\EFI\ubuntu\grubx64.efi`（不包括文件系统标识符）。

![image-20240801113910982](D:\WorkFile\EveryDay\虚拟机开机时出现Press ESC in 5 seconds to skip startup.nsh, any other key to continue问题的解决办法.assets\image-20240801113910982.png)

```
`\EFI\centos\grubaa64.efi`
```

