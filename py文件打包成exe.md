## py文件打包成exe

### **普通打包**

1、第一步下载pyinstaller，执行：pip install pyinstaller

2、第二步打包，执行：pyinstaller -w -F -i tb.ico xxx.py

```
-w 表示希望在生成的.exe程序运行过程中，不要出现cmd黑框，一般用于打包GUI界面

-F：表示希望将所有的程序全部打包在一起，生成的只有一个.exe文件，这样的文件集成度高，但是运行速度慢

　　如果不写-F，生成的还有一堆.dll文件，这样的程序里文件很多，但是运行速度比较快

-D：生成一个文件目录包含可执行文件和相关动态链接库和资源文件等

对于打包结果较大的项目，选用-d生成目录相比单可执行文件的打包方式，执行速度更快，但包含更加多的文件。

-p：自定义需要加载的类的路径

-i：自己做的软件可以放上自己的图标，分享一个网站，可以把其他格式图片转成ico格式：https://app.xunjiepdf.com/img2icon/

XXX.py：指的是你整个项目的入口程序，大家写项目时很可能是多文件编程，你整个项目时靠哪个文件作为主入口拉起来的，就填哪个文件的名字
```

举例:

```
pyinstall -F xxxx.py  打包exe

pyinstall -F -w xxxx.py   不带控制台的打包

pyinstall -F -w -i tb.ico xxxx.py  指定exe图标打包，tb是图标文件名
```



### 多个文件打包

> 使用spec方式





1. 打开终端进入项目路径下，输入 `pyinstaller -F -c docx2xml.py(项目主文件)` 。执行后，会在当前目录下生成两个文件夹（build和dist）和1个文件 `docx2xml.spec` 。删除那两个文件夹，只保留 `docx2xml.spec`。

   > 做个说明，这里的 `pyinstaller -F -c docx2xml.py(项目主文件)` 可以写成 `pyinstaller -F -w docx2xml.py(项目主文件)`。只不过 `-c` 的在执行打包后的exe文件时会带控制台，也就是DOS窗口，而 `-w` 不带控制台。在第一次打包时，如果程序打包后有错误，用 `-c` 的可以在执行打包后的exe文件时直观地看到程序报错信息。

2. 根据自己的项目编辑该文件

   ```python
   # -*- mode: python ; coding: utf-8 -*-
   
   
   block_cipher = None
   
   #此项目中所有的py文件（要打包进去的所有py文件），和主程序不在同一个包中的py文件用绝对路径。
   a = Analysis(['docx2xml.py','content.py','extract.py','head.py','image.py','processXml.py',
                   'D:/Desktop/PaperSystem-Python/lib/log.py',
                   'D:/Desktop/PaperSystem-Python/lib/misc.py',
                   'D:/Desktop/PaperSystem-Python/lib/procXml.py',
                   'D:/Desktop/PaperSystem-Python/lib/styleEnum.py',
               ],
                pathex=['D:/Desktop/PaperSystem-Python/doc2xml'],	# 项目的绝对路径
                binaries=[],
                datas=[],
                hiddenimports=[],
                hookspath=[],
                hooksconfig={},
                runtime_hooks=[],
                excludes=[],
                win_no_prefer_redirects=False,
                win_private_assemblies=False,
                cipher=block_cipher,
                noarchive=False)
   pyz = PYZ(a.pure, a.zipped_data,
                cipher=block_cipher)
   
   exe = EXE(pyz,
             a.scripts,
             a.binaries,
             a.zipfiles,
             a.datas,  
             [],
             name='docx2xml',	#打包程序的名字
             debug=False,
             bootloader_ignore_signals=False,
             strip=False,
             upx=True,
             upx_exclude=[],
             runtime_tmpdir=None,
             # console=True表示，打包后的可执行文件双击运行时屏幕会出现一个cmd窗口，不影响原程序运行
             console=True,
             disable_windowed_traceback=False,
             target_arch=None,
             codesign_identity=None,
             entitlements_file=None )
   
   # 如果想要修改程序图标，使用在EXE()中加入 icon='xxxxx', 切记：绝对路径
   
   ```

3. 打包

   执行spec文件：

   ```python
   pyinstaller -F -c docx2xml.spec
   ```

   运行结束后，会新增dist文件夹，exe文件就在里面。（该exe文件已将所有的py文件封装完了，可随处使用）。

































