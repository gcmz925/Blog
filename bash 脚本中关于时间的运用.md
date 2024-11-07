## bash 脚本中关于时间的运用

```bash
@echo off
REM 记录当前开始时间
set start_time=%time%

echo Start time: %start_time%
REM 模拟某些操作，例如等待 5 秒（这里可以替换成其他操作）
timeout /t 5 > nul

REM 记录结束时间
set end_time=%time%
echo End time: %end_time%

REM 将开始和结束时间转换为秒数
call :TimeToSeconds start_seconds %start_time%
call :TimeToSeconds end_seconds %end_time%

REM 计算时间差（秒）
set /a elapsed_seconds=%end_seconds%-%start_seconds%

REM 如果时间差为负值（表示跨天了），则加上 86400 秒 (24 小时)
if %elapsed_seconds% lss 0 set /a elapsed_seconds+=86400

REM 将时间差转换回时:分:秒格式
call :FormatElapsedTime %elapsed_seconds%

pause
exit /b

REM 函数：将 HH:MM:SS 时间格式转换为秒数
:TimeToSeconds
setlocal
set time_string=%~2
set /a hours=1%time_string:~0,2% - 100
set /a minutes=1%time_string:~3,2% - 100
set /a seconds=1%time_string:~6,2% - 100
set /a %1=hours*3600 + minutes*60 + seconds
endlocal & set %1=%hours% * 3600 + %minutes% * 60 + %seconds%
exit /b

REM 函数：将秒数转换为时:分:秒格式输出
:FormatElapsedTime
setlocal
set /a seconds=%1, minutes=seconds/60, hours=minutes/60
set /a seconds=seconds%%60, minutes=minutes%%60
echo Elapsed Time: %hours% hour(s), %minutes% minute(s), %seconds% second(s)
exit /b

```





这些语句是用于将时间字符串中的小时、分钟和秒提取出来，并转换为整数表示。具体解释如下：

### 代码解析
```bat
REM 函数：将 HH:MM:SS 时间格式转换为秒数
:TimeToSeconds
setlocal
set time_string=%~2
set /a hours=1%time_string:~0,2% - 100
set /a minutes=1%time_string:~3,2% - 100
set /a seconds=1%time_string:~6,2% - 100
set /a %1=hours*3600 + minutes*60 + seconds
endlocal & set %1=%hours% * 3600 + %minutes% * 60 + %seconds%
exit /b
```

1. **`set time_string=%~2`**:
   
   - `set time_string=%~2` 语句将传递给当前批处理脚本的第二个参数（`%2`）赋值给变量 `time_string`。`%~2` 表示去除参数 `%2` 可能包含的引号。
- 例如，如果批处理脚本被调用为 `myScript.bat "12:34:56" 15:43:21`，那么 `%2` 对应的值就是 `15:43:21`，`time_string` 变量的值将被设置为 `15:43:21`。
   
2. **`set /a hours=1%time_string:~0,2% - 100`**:
   - 这一行从 `time_string` 中提取 `0` 到 `2` 的字符（即小时部分），并将其转换为整数：
     - `time_string:~0,2` 提取了 `time_string` 中从索引 `0` 开始的两个字符，表示小时数（假设 `time_string` 为 `15:43:21`，则提取结果为 `15`）。
     - `1%time_string:~0,2%` 在前面加了一个 `1`，避免批处理将 `08` 和 `09` 这样的数字视为八进制。
     - `- 100` 用来去除多加的 `1`。比如 `115 - 100 = 15`，`08` 会变成 `108 - 100 = 8`，这样处理后就得到了小时数的整数表示。
   - 最终，`hours` 变量会被赋值为整数形式的小时数。

3. **`set /a minutes=1%time_string:~3,2% - 100`**:
   - 这一行从 `time_string` 中提取 `3` 到 `5` 的字符（即分钟部分），并将其转换为整数：
     - `time_string:~3,2` 提取了分钟部分，比如 `15:43:21` 的分钟部分是 `43`。
     - 同样在前面加 `1`，然后再减 `100` 来去除前导 `1`。
   - 最终，`minutes` 变量会被赋值为整数形式的分钟数。

4. **`set /a seconds=1%time_string:~6,2% - 100`**:
   
   - 这一行从 `time_string` 中提取 `6` 到 `8` 的字符（即秒部分），并将其转换为整数：
     - `time_string:~6,2` 提取了秒数部分，比如 `15:43:21` 的秒部分是 `21`。
     - 同样在前面加 `1`，然后再减 `100` 来去除前导 `1`。
- 最终，`seconds` 变量会被赋值为整数形式的秒数。
   
5. **`set /a %1=hours*3600 + minutes*60 + seconds`**:
   
   - 这一行将提取的小时、分钟和秒数转换为总秒数，并赋值给传递给批处理脚本的第一个参数（`%1`）。
   - 例如，如果 `hours=15`，`minutes=43`，`seconds=21`，则计算的总秒数为：
     ```bat
     set /a %1=15*3600 + 43*60 + 21
     ```
     计算结果为 `56580 + 2580 + 21 = 59181`，最终 `59181` 秒会被赋值给 `%1` 所指向的变量（假设 `%1` 对应的变量名是 `elapsed_seconds`，那么 `elapsed_seconds` 就会被赋值为 `59181`）。



### 问:设置两次原因

```bat
set /a %1=hours*3600 + minutes*60 + seconds
endlocal & set %1=%hours% * 3600 + %minutes% * 60 + %seconds%
```

> ### 背景与原因
>
> 在批处理脚本中，`setlocal` 和 `endlocal` 用于创建和销毁局部变量作用域。也就是说，任何在 `setlocal` 和 `endlocal` 之间创建或修改的变量，都会在 `endlocal` 执行之后失效，返回到 `setlocal` 之前的状态。这使得它们非常适合防止变量污染全局环境。
>
> ### 具体原因
>
> 1. **`setlocal` 与 `endlocal` 作用域**：
>    - 如果你在 `setlocal` 之后定义的变量（比如 `hours`、`minutes`、`seconds`），当执行 `endlocal` 后，这些变量将失效。
>    - 当 `endlocal` 执行后，如果想要将 `setlocal` 作用域中的局部变量的值传递到全局变量，就需要使用一些技巧。
> 2. **为什么用两次 `set`**：
>    - 在 `endlocal` 之后，如果想将局部变量的值传递到全局变量，需要在 `endlocal` 后立即执行 `set` 命令，因为 `endlocal` 会让局部变量失效。
>    - 由于 `endlocal` 会立即生效，直接用 `set` 的话，局部变量 `hours`、`minutes` 和 `seconds` 会在 `endlocal` 执行后失效，从而导致无法正确将其值赋值给全局变量。因此，使用 `endlocal & set` 语法来在执行 `endlocal` 之前保存变量的值，然后在 `set` 中应用这些值。
>
> 





### 问:运算结果不符

> ### 问题现象
>
> 其实上述脚本还存在问题,当bat脚本跑起来后,发现计算的值与实际不符,最后在 TimeToSeconds 函数逐步echo发现,最后一步
>
> ```bash
> set /a %1=hours*3600 + minutes*60 + seconds
> ```
>
> 这一步这里不会计算导致出现问题，
>
> ```
> D:\WorkFile\20241008>test.bat
> Start time: 10:24:18.30
> End time: 10:24:23.19
> Elapsed Time: 0 hour(s), 48 minute(s), 41 second(s)
> 请按任意键继续. . .
> ```
>
> ### 原因分析
>
> ```bat
> endlocal & set %1=%hours% * 3600 + %minutes% * 60 + %seconds%
> ```
>
> 这行代码实际上会把 `%hours% * 3600 + %minutes% * 60 + %seconds%` 这个表达式当作**字符串**赋值给变量 `%1`，而不会进行实际的数学运算。原因是 `set` 命令默认是字符串操作，除非用 `/a` 参数来强制进行算术运算（如：`set /a`）。
>
> 例如：
>
> - 假设 `hours=2`，`minutes=30`，`seconds=15`。
> - 执行上面的语句时，它不会计算 `2 * 3600 + 30 * 60 + 15` 的结果，而是直接将字符串 `"2 * 3600 + 30 * 60 + 15"` 赋值给变量 `%1`。
> - 当你试图 `echo` 或使用 `%1` 变量时，它的内容是 `"2 * 3600 + 30 * 60 + 15"`（而不是 `9015`），所以不能得到正确的结果。
>
> ### 解决方法
>
> 如果希望在 `endlocal` 之后将计算结果传递给全局变量，可以使用下面的技巧：
>
> 1. **在 `endlocal` 之前计算出结果**，并通过命令行传递到 `endlocal` 之后。
> 2. 使用 **临时文件** 或其他方法将值传递到 `endlocal` 之后。
>
> ### 方法 1：通过 `setlocal` 中的临时变量传递值
>
> 一种常见的做法是先用一个临时变量存储计算结果，然后使用 `endlocal &` 语法将该值传递给全局变量：
>
> ```bat
> @echo off
> setlocal
> set hours=2
> set minutes=30
> set seconds=15
> 
> REM 在局部变量范围内计算总秒数
> set /a total_seconds=hours*3600 + minutes*60 + seconds
> 
> REM 通过临时变量传递总秒数
> endlocal & set total_seconds=%total_seconds%
> 
> echo Total seconds: %total_seconds%
> pause
> ```
>
> ### 解析
>
> 1. **局部变量计算**：
>    - 在 `setlocal` 范围内，计算了 `total_seconds` 的值（即 `2*3600 + 30*60 + 15 = 9015`）。
> 2. **传递临时变量**：
>    - 使用 `endlocal & set total_seconds=%total_seconds%`，将 `total_seconds` 的值从 `setlocal` 范围内传递到全局变量中。
>    - 在执行 `endlocal` 之前，`%total_seconds%` 的值就已经被解析为 `9015`（局部作用域中计算出的值）。
> 3. **输出结果**：
>    - `total_seconds` 的全局变量现在已经是 `9015`，因此可以在 `echo` 中正确输出。
>
> ### 方法 2：使用 `setlocal` 和 `endlocal` 同时传递多个变量
>
> 假设你希望将多个变量值传递到全局作用域，可以采用如下方式：
>
> ```bat
> @echo off
> setlocal
> set hours=2
> set minutes=30
> set seconds=15
> 
> REM 计算各变量的总秒数
> set /a total_seconds=hours*3600 + minutes*60 + seconds
> 
> REM 使用endlocal传递多个变量的值
> endlocal & (
>     set "hours=%hours%" ^
>     & set "minutes=%minutes%" ^
>     & set "seconds=%seconds%" ^
>     & set "total_seconds=%total_seconds%"
> )
> 
> echo Hours: %hours%
> echo Minutes: %minutes%
> echo Seconds: %seconds%
> echo Total seconds: %total_seconds%
> pause
> ```
>
> ### 解析
>
> 1. **局部变量计算**：
>    - 在 `setlocal` 范围内，定义了 `hours`、`minutes`、`seconds` 并计算了 `total_seconds`。
> 2. **同时传递多个变量**：
>    - 使用 `endlocal & (...)` 的方式，传递了多个变量值。
>    - `^` 是转义符，用于表示当前行没有结束，下一行仍然属于同一个命令块。
>    - `set "var=value"` 中使用双引号，可以确保变量值中包含空格或其他特殊字符时能够正确处理。
> 3. **输出结果**：
>    - `hours`、`minutes`、`seconds` 和 `total_seconds` 变量都被成功传递到了全局作用域，因此可以正确输出它们的值。
>
> ### 方法 3：使用 `call` 传递计算结果
>
> 另一种方法是使用 `call` 命令，通过子例程来传递计算后的值。例如：
>
> ```bat
> @echo off
> setlocal
> set hours=2
> set minutes=30
> set seconds=15
> 
> REM 在局部变量中计算秒数
> set /a total_seconds=hours*3600 + minutes*60 + seconds
> 
> REM 调用子例程并传递计算结果
> call :SetGlobalVar total_seconds %total_seconds%
> 
> endlocal
> 
> echo Total seconds: %total_seconds%
> pause
> exit /b
> 
> :SetGlobalVar
> set %1=%2
> exit /b
> ```
>
> ### 解析
>
> 1. **局部变量计算**：
>    - 在 `setlocal` 范围内计算 `total_seconds`，得到的值为 `9015`。
> 2. **通过子例程传递结果**：
>    - 使用 `call :SetGlobalVar` 传递计算后的值。
>    - `call :SetGlobalVar total_seconds %total_seconds%` 将 `9015` 的值传递给全局变量 `total_seconds`。
> 3. **输出结果**：
>    - `total_seconds` 的全局变量被设置为 `9015`，因此可以在 `endlocal` 后成功输出。
>
> ### 总结
>
> - **直接用 `set %1=%hours% \* 3600 + %minutes% \* 60 + %seconds%` 不会进行数学运算**，而是将表达式作为字符串进行赋值。
> - 为了解决这个问题，可以先在 `setlocal` 中计算出结果，然后通过 `endlocal & set` 或 `call` 的方式传递计算后的值到全局作用域。
> - 以上几种方法可以根据需求来选择使用，确保局部变量的值能够在 `endlocal` 之后正确传递到全局变量中。
>
> ### 修改举例:
>
> ```bat
> REM 函数：将 HH:MM:SS 时间格式转换为秒数
> :TimeToSeconds
> setlocal
> set time_string=%~2
> set /a hours=1%time_string:~0,2% - 100
> set /a minutes=1%time_string:~3,2% - 100
> set /a seconds=1%time_string:~6,2% - 100
> set /a total_seconds=hours*3600 + minutes*60 + seconds
> endlocal & set %1=%total_seconds%
> exit /b
> ```
>
> 







