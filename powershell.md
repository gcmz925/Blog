

## 1.Set-ExecutionPolicy 有效范围

> powershell.exe Set-ExecutionPolicy Bypass   这个运行是永久有效的吗



在 PowerShell 中，`Set-ExecutionPolicy` 命令更改的是执行策略，这决定了脚本和配置文件是否可以被执行以及如何执行。但这个命令的更改 **不一定是永久有效的**，它取决于你设置的作用范围（Scope）。

### 执行策略的作用范围（Scope）

PowerShell 的执行策略可以在不同的范围（Scope）下设置，每个范围都有不同的持久性和优先级。常见的范围有：

1. **LocalMachine**：适用于本地计算机上的所有用户。这个是永久的设置，除非手动更改。
2. **CurrentUser**：只影响当前用户。对于当前用户是永久的，除非你在同一用户下更改。
3. **Process**：只对当前 PowerShell 会话有效。一旦你关闭 PowerShell 终端或进程，设置会消失。
4. **UserPolicy/MachinePolicy**：这些是通过组策略设置的，通常由系统管理员控制。

### 默认情况下的作用范围

如果你使用 `Set-ExecutionPolicy Bypass`，但 **没有指定作用范围**，它会默认修改 **LocalMachine** 作用范围，也就是对本地所有用户生效，并且是永久的，直到你手动更改它。

### 如何确保设置永久生效

如果你希望这个策略对所有用户永久生效，你可以显式指定作用范围为 `LocalMachine`：

```powershell
Set-ExecutionPolicy Bypass -Scope LocalMachine
```

或者，如果你只希望它对当前用户永久生效：

```powershell
Set-ExecutionPolicy Bypass -Scope CurrentUser
```

### 如何查看当前执行策略

你可以使用以下命令查看当前的执行策略以及不同作用范围下的执行策略：

```powershell
Get-ExecutionPolicy -List
```

这将列出 `MachinePolicy`、`UserPolicy`、`Process`、`CurrentUser` 和 `LocalMachine` 的执行策略。

### 总结
- 如果你运行 `Set-ExecutionPolicy Bypass` 而不指定作用范围，默认会设置为 `LocalMachine`，这意味着该设置是永久的。
- 如果你只想在当前会话中临时绕过执行策略，可以指定 `Process` 作用范围：

  ```powershell
  Set-ExecutionPolicy Bypass -Scope Process
  ```

关闭 PowerShell 终端后，`Process` 作用范围的更改会被自动清除。





## 2.Set-ExecutionPolicy是否写入注册表

PowerShell 的执行策略确实会写入到注册表中，尤其是当你使用 `Set-ExecutionPolicy` 并作用于 `LocalMachine` 或 `CurrentUser` 范围时。这些设置存储在 Windows 注册表中。你可以通过注册表手动查看或更改这些设置，但建议主要通过 PowerShell 命令管理执行策略。

### 注册表路径

PowerShell 的执行策略在以下注册表位置存储：

- **LocalMachine（适用于所有用户的全局设置）**：
  
  ```
  HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\PowerShell\1\ShellIds\Microsoft.PowerShell
  ```
键名：`ExecutionPolicy`
  
- **CurrentUser（适用于当前用户的设置）**：
  
  ```
  HKEY_CURRENT_USER\SOFTWARE\Microsoft\PowerShell\1\ShellIds\Microsoft.PowerShell
  ```
  键名：`ExecutionPolicy`

### 如何查看注册表中的执行策略

可以使用 `regedit` 查看上述注册表路径中的 `ExecutionPolicy` 值：

1. 按 `Win + R`，输入 `regedit`，然后按回车。
2. 导航到上述路径，根据你要查看的是 `LocalMachine` 还是 `CurrentUser`。
3. 在右侧窗口中查找名为 `ExecutionPolicy` 的键，它的值会显示当前设置的执行策略。

### 使用 PowerShell 查询注册表

如果更倾向于使用 PowerShell 直接查询注册表中的执行策略，你可以运行以下命令：

- 查看 **LocalMachine** 执行策略：
  ```powershell
  Get-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\PowerShell\1\ShellIds\Microsoft.PowerShell' | Select-Object ExecutionPolicy
  ```

- 查看 **CurrentUser** 执行策略：
  ```powershell
  Get-ItemProperty -Path 'HKCU:\SOFTWARE\Microsoft\PowerShell\1\ShellIds\Microsoft.PowerShell' | Select-Object ExecutionPolicy
  ```

### `Get-ExecutionPolicy` 和注册表的关系

- `Get-ExecutionPolicy -List` 显示的值是通过查询注册表中相应的键，以及当前进程范围的设置，来确定执行策略的。它列出了所有作用范围的策略（`Process`、`CurrentUser`、`LocalMachine` 等）。
- 当运行 `Set-ExecutionPolicy` 并作用于 `LocalMachine` 或 `CurrentUser` 时，它将自动写入到相应的注册表位置。

### 总结

- 执行策略确实存储在注册表中，可以手动查看或通过 PowerShell 查询这些注册表值。
- 使用 `Get-ExecutionPolicy -List` 是最方便的方式来查看当前生效的执行策略，但也可以通过注册表来确认。





## 3.执行策略的优先级是怎么样的

**`LocalMachine`** 和 **`CurrentUser`** 的执行策略可以同时存在，但它们的优先级不同。当 PowerShell 确定执行策略时，会根据作用范围按以下优先级来决定哪个策略生效：

### 执行策略的优先级顺序

从最高到最低优先级的顺序如下：

1. **Group Policy (`MachinePolicy` 和 `UserPolicy`)**：如果系统管理员通过组策略设置了执行策略，它会覆盖所有其他设置。这些策略通常由 IT 管理员通过组策略部署。
2. **Process**：如果你在当前 PowerShell 会话中使用 `-Scope Process` 设置了执行策略，它只对该会话生效，优先级高于 `CurrentUser` 和 `LocalMachine`。
3. **CurrentUser**：仅对当前用户生效。它比 `LocalMachine` 优先级高。
4. **LocalMachine**：对本地计算机的所有用户生效。如果没有更高优先级的策略，这个策略会生效。

### 当 `LocalMachine` 和 `CurrentUser` 都存在时

- 如果同时设置了 **`CurrentUser`** 和 **`LocalMachine`** 的执行策略，那么 **`CurrentUser`** 的策略优先于 **`LocalMachine`** 的策略。
- 也就是说，如果你在 **`CurrentUser`** 作用范围设置了 `RemoteSigned`，而 **`LocalMachine`** 设置了 `Restricted`，那么 PowerShell 将会使用 `CurrentUser` 的 `RemoteSigned` 执行策略。

### 示例：查看当前所有作用范围的执行策略

可以使用以下命令查看不同作用范围的执行策略：

```powershell
Get-ExecutionPolicy -List
```

示例输出：

```
        Scope ExecutionPolicy
        ----- ---------------
MachinePolicy       Undefined
   UserPolicy       Undefined
      Process       Undefined
  CurrentUser      RemoteSigned
 LocalMachine      Restricted
```

在这个示例中：
- **`CurrentUser`** 设置为 `RemoteSigned`，优先于 **`LocalMachine`** 的 `Restricted`，因此 `RemoteSigned` 是当前生效的策略。
- 如果在当前 PowerShell 会话中使用了 `Set-ExecutionPolicy Bypass -Scope Process`，那么 `Bypass` 将是临时生效的策略，直到关闭 PowerShell 会话。

### 总结

- **`LocalMachine`** 和 **`CurrentUser`** 的执行策略可以同时存在，但 **`CurrentUser`** 优先于 **`LocalMachine`**。
- 如果你没有为 **`CurrentUser`** 设置执行策略，PowerShell 会回退到 **`LocalMachine`** 的执行策略。
- 你可以使用 `Get-ExecutionPolicy -List` 来查看所有作用范围下的策略。





## 4.python 编码判断当前策略

可以编写一个 Python 脚本来查询 Windows 注册表中 `LocalMachine` 和 `CurrentUser` 的 PowerShell 执行策略，并根据优先级处理。Python 的 `winreg` 模块可以用来访问注册表。

这个脚本会读取注册表中的执行策略，按照优先级（`CurrentUser` 优先于 `LocalMachine`）判断当前生效的策略。如果某个作用范围没有设置策略，会返回 `None`。

### Python 脚本：

```python
import winreg

# 注册表路径
LOCAL_MACHINE_KEY = r"SOFTWARE\Microsoft\PowerShell\1\ShellIds\Microsoft.PowerShell"
CURRENT_USER_KEY = r"SOFTWARE\Microsoft\PowerShell\1\ShellIds\Microsoft.PowerShell"

def get_execution_policy_from_registry(hive, sub_key):
    try:
        with winreg.OpenKey(hive, sub_key) as key:
            # 读取 ExecutionPolicy 值
            value, regtype = winreg.QueryValueEx(key, "ExecutionPolicy")
            return value
    except FileNotFoundError:
        # 如果键不存在，返回 None
        return None

def get_execution_policy():
    # 获取 LocalMachine 执行策略
    local_machine_policy = get_execution_policy_from_registry(winreg.HKEY_LOCAL_MACHINE, LOCAL_MACHINE_KEY)
    
    # 获取 CurrentUser 执行策略
    current_user_policy = get_execution_policy_from_registry(winreg.HKEY_CURRENT_USER, CURRENT_USER_KEY)

    # 判断优先级：CurrentUser > LocalMachine
    if current_user_policy:
        print("CurrentUser ExecutionPolicy: {}".format(current_user_policy))
        return current_user_policy
    elif local_machine_policy:
        print("LocalMachine ExecutionPolicy: {}".format(local_machine_policy))
        return local_machine_policy
    else:
        print("No execution policy set for LocalMachine or CurrentUser.")
        return None

if __name__ == "__main__":
    current_policy = get_execution_policy()
    if current_policy:
        print("Active ExecutionPolicy: {}".format(current_policy))
    else:
        print("No active execution policy found.")

```

### 脚本说明：

1. **`get_execution_policy_from_registry`**：此函数用来从注册表中读取指定路径的 `ExecutionPolicy`。它尝试打开注册表中的键，并返回其值，如果键不存在（比如该作用范围没有设置执行策略），则返回 `None`。
   
2. **`get_execution_policy`**：这个函数会首先尝试读取 `CurrentUser` 的策略，如果存在则返回该策略。否则，它会返回 `LocalMachine` 的策略。如果两个都没有设置，返回 `None`。

3. **`winreg` 模块**：这是 Python 内置的模块，用于访问和读取 Windows 注册表。

### 执行结果：

- 如果 **`CurrentUser`** 存在，则返回 `CurrentUser` 的执行策略。
- 如果 **`CurrentUser`** 不存在，返回 **`LocalMachine`** 的执行策略。
- 如果两个作用范围都没有设置，则输出提示。

### 运行示例：

假设你运行这个脚本，且 `CurrentUser` 执行策略为 `RemoteSigned`，`LocalMachine` 执行策略为 `Restricted`，输出将类似于：

```
CurrentUser ExecutionPolicy: RemoteSigned
Active ExecutionPolicy: RemoteSigned
```

如果 `CurrentUser` 没有设置策略，而 `LocalMachine` 设置为 `Restricted`，则输出：

```
LocalMachine ExecutionPolicy: Restricted
Active ExecutionPolicy: Restricted
```

如果两个都没有设置，则输出：

```
No execution policy set for LocalMachine or CurrentUser.
No active execution policy found.
```





## 5.都没有设置执行策略，策略定义

如果 **`LocalMachine`** 和 **`CurrentUser`** 作用范围都没有设置执行策略，那么 PowerShell 将使用其默认的执行策略。PowerShell 不同版本的默认执行策略可能有所不同：

- **Windows PowerShell**（经典版本）：默认策略是 **`Restricted`**。

因此，如果两个作用范围都没有设置，PowerShell 将会使用这个默认策略。在 Windows 上通常是 **`Restricted`**。

### PowerShell 执行策略类型

PowerShell 执行策略有多种类型，用于确定脚本的执行权限和安全性。以下是几种常见的执行策略类型：

1. **Restricted**（限制）：不允许任何脚本运行。这是最严格的策略，只允许运行单个命令，不允许运行脚本文件（`.ps1`）。
   
2. **AllSigned**（全部签名）：只能运行经过数字签名的脚本和配置文件。PowerShell 将在运行前提示你确认是否信任签名。

3. **RemoteSigned**（远程签名）：只能运行本地创建的脚本或已签名的远程脚本。通过互联网下载的脚本必须由受信任的发布者签名。

4. **Unrestricted**（无限制）：允许运行任何脚本。在执行从互联网下载的脚本时，PowerShell 会警告你脚本来自不受信任的来源，但允许你选择是否继续执行。

5. **Bypass**（绕过）：没有任何限制，也不会发出警告或提示。此策略用于希望完全跳过执行策略检查的场景。

6. **Undefined**（未定义）：此作用范围没有定义执行策略。如果 `CurrentUser` 或 `LocalMachine` 的执行策略是 `Undefined`，PowerShell 会使用默认的执行策略。

### 查询当前的执行策略

你可以通过以下命令查看所有作用范围的执行策略：

```powershell
Get-ExecutionPolicy -List
```

输出示例：

```
        Scope ExecutionPolicy
        ----- ---------------
MachinePolicy       Undefined
   UserPolicy       Undefined
      Process       Undefined
  CurrentUser       Undefined
 LocalMachine       Restricted
```

在这个例子中，`LocalMachine` 设置为 `Restricted`，而其他作用范围都未定义，因此 `Restricted` 生效。

### 执行策略优先级回顾

1. **Group Policy**（`MachinePolicy`、`UserPolicy`）—— 最高优先级（通常用于系统管理员设置的策略）。
2. **Process**—— 仅在当前 PowerShell 会话中有效，优先级高于 `CurrentUser` 和 `LocalMachine`。
3. **CurrentUser**—— 针对当前用户，优先级高于 `LocalMachine`。
4. **LocalMachine**—— 默认设置，适用于所有用户。

### 总结

- 如果两个作用范围（`LocalMachine` 和 `CurrentUser`）都没有设置执行策略，PowerShell 将使用默认策略。
- **Windows PowerShell** 默认执行策略为 **`Restricted`**，**PowerShell Core** 默认执行策略为 **`RemoteSigned`**。
- 你可以使用 `Get-ExecutionPolicy -List` 来查看当前生效的策略，并根据优先级决定哪个策略最终生效。