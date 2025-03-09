关于 UNICODE_STRING 的操作

在驱动程序中，Unicode 字符串的操作通常涉及使用 Windows 内核提供的 Unicode 类型和相关的函数。Windows 内核提供了 `UNICODE_STRING` 类型来表示 Unicode 字符串，操作这些字符串时需要注意内存分配、字符串转换以及相关的 API 调用。以下是一些常见的操作示例：

### 1. 定义 `UNICODE_STRING`

`UNICODE_STRING` 是 Windows 内核中表示 Unicode 字符串的结构体，通常包含一个指向字符串数据的指针和字符串的长度信息。

```c
UNICODE_STRING unicodeString;
RtlInitUnicodeString(&unicodeString, L"Hello, Kernel World!");
```

### 2. 初始化 `UNICODE_STRING`

初始化 `UNICODE_STRING` 字符串，使用 `RtlInitUnicodeString` 或 `RtlUnicodeStringInit`。这两个函数的差异在于参数的传递形式。

```c
UNICODE_STRING unicodeStr;
RtlInitUnicodeString(&unicodeStr, L"Example String");
```

### 3. 将 `UNICODE_STRING` 转换为常规 C 字符串

如果你需要将 `UNICODE_STRING` 转换为常规 C 字符串，可以通过 `unicodeString.Buffer` 来访问其字符内容。

```c
wchar_t* str = unicodeString.Buffer;
```

### 4. 比较 `UNICODE_STRING` 字符串

`RtlCompareUnicodeString` 用于比较两个 `UNICODE_STRING` 字符串。

```c
UNICODE_STRING str1, str2;
RtlInitUnicodeString(&str1, L"Test1");
RtlInitUnicodeString(&str2, L"Test2");

NTSTATUS status = RtlCompareUnicodeString(&str1, &str2, TRUE);
if (status == 0) {
    // 字符串相等
}
```

### 5. 复制 `UNICODE_STRING`

`RtlCopyUnicodeString` 用于将一个 `UNICODE_STRING` 的内容复制到另一个 `UNICODE_STRING` 中。

```c
UNICODE_STRING src, dest;
RtlInitUnicodeString(&src, L"Source String");
RtlZeroUnicodeString(&dest);
RtlCopyUnicodeString(&dest, &src);
```

### 6. 连接 `UNICODE_STRING`

`RtlAppendUnicodeStringToString` 用于将一个字符串追加到另一个字符串的末尾。

```c
UNICODE_STRING str1, str2;
RtlInitUnicodeString(&str1, L"Hello");
RtlInitUnicodeString(&str2, L"World");

RtlAppendUnicodeStringToString(&str1, &str2);
// str1 现在为 "HelloWorld"
```

### 7. 转换 `UNICODE_STRING` 到大写/小写

`RtlUpcaseUnicodeString` 将字符串转换为大写，`RtlDowncaseUnicodeString` 将其转换为小写。

```c
UNICODE_STRING str;
RtlInitUnicodeString(&str, L"Lowercase String");

RtlUpcaseUnicodeString(&str, &str, FALSE);  // 转换为大写
RtlDowncaseUnicodeString(&str, &str, FALSE); // 转换为小写
```

### 8. 分配和释放内存

为了避免内存泄漏，内存分配需要使用 `ExAllocatePoolWithTag` 或类似的内存分配函数。

```c
UNICODE_STRING str;
RtlZeroMemory(&str, sizeof(UNICODE_STRING));

str.Buffer = ExAllocatePoolWithTag(NonPagedPool, sizeof(wchar_t) * (length + 1), 'Tag'); // 分配内存
str.Length = length * sizeof(wchar_t);
str.MaximumLength = str.Length + sizeof(wchar_t);  // 包括末尾的空字符

RtlInitUnicodeString(&str, L"Some String");

// 使用后释放内存
ExFreePoolWithTag(str.Buffer, 'Tag');
```

### 9. 将 `UNICODE_STRING` 转换为 `ANSI_STRING`

如果需要将 `UNICODE_STRING` 转换为 `ANSI_STRING`，可以使用 `RtlUnicodeStringToAnsiString`。注意，这可能需要为转换后的字符串分配内存。

```c
ANSI_STRING ansiStr;
UNICODE_STRING unicodeStr;
RtlInitUnicodeString(&unicodeStr, L"Example");

RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);  // 第二个参数为TRUE表示分配内存

// 使用后释放内存
RtlFreeAnsiString(&ansiStr);
```

### 10. 获取字符串的长度

`UNICODE_STRING` 结构本身就有 `Length` 字段表示字符串的长度，单位是字节。如果需要单位是字符数，除以 `sizeof(wchar_t)`。

```c
ULONG len = unicodeString.Length / sizeof(wchar_t);  // 获取字符数
```

这些是一些典型的 Unicode 字符串操作，适用于驱动开发中的常见需求。根据实际需求，你可以选择合适的 API 来操作 `UNICODE_STRING`。