关于create是否是目录

**`Iopb->TargetFileObject->FileName`**：这是一个`UNICODE_STRING`类型，包含了目标文件对象的文件名。你可以通过检查文件名的特征（例如是否以反斜杠结尾）来推断它是文件还是目录。

**`Iopb->MajorFunction`**：这是一个指定主要函数的字段，表示这次操作的类型。例如，创建、读取、写入等操作。对于创建操作，可以进一步检查它是针对文件还是目录。

**`Iopb->Parameters.Create.Options`**：这是一个包含创建选项的字段。可以通过检查其中的标志位来确定目标是文件还是目录。具体来说：

- `FILE_DIRECTORY_FILE`：表示目标是一个目录。 0x00000001
- `FILE_NON_DIRECTORY_FILE`：表示目标是一个文件。 0x00000040



该方法存在问题

原因是：

当 IRP_MJ_CREATE 操作创建目录的时候，一定要设置 FILE_DIRECTORY_FILE 标志，打开目录的时候，却不需要。

因此在打开目录的时候，上面的判断方法就不能够很好的工作。


如何设置了  FILE_DIRECTORY_FILE 标志去打开普通的文件，则操作会失败。


---------------------------------------------------

FltIsDirectory 可以很好的解决这个问题，不过需要在IRP_MJ_CREATE 的 PostOperation 中调用。








**想从另外一个角度解决，既是 存在 FILE_DIRECTORY_FILE ，又只有创建的权限**



Parameters.Create.Options成员是一个 ULONG 值，描述打开句柄时使用的选项。高 8 位对应于**ZwCreateFile**的*CreateDisposition参数的值，低 24***位对应于****ZwCreateFile**的*CreateOptions*参数的值。



```
[in] CreateDisposition
```

Specifies the action to perform if the file does or does not exist. *CreateDisposition* can be one of the values in the following table.

| *CreateDisposition* value | Action if file exists            | Action if file does not exist |
| :------------------------ | :------------------------------- | :---------------------------- |
| FILE_SUPERSEDE            | Replace the file.                | Create the file.              |
| FILE_CREATE               | Return an error.                 | Create the file.              |
| FILE_OPEN                 | Open the file.                   | Return an error.              |
| FILE_OPEN_IF              | Open the file.                   | Create the file.              |
| FILE_OVERWRITE            | Open the file, and overwrite it. | Return an error.              |
| FILE_OVERWRITE_IF         | Open the file, and overwrite it. | Create the file.              |





```
[in] CreateOptions
```

Specifies the options to apply when the driver creates or opens the file. Use one or more of the flags in the following table.

| *CreateOptions* flag    | Meaning                                                      |
| :---------------------- | :----------------------------------------------------------- |
| FILE_DIRECTORY_FILE     | The file is a directory. Compatible *CreateOptions* flags are FILE_SYNCHRONOUS_IO_ALERT, FILE_SYNCHRONOUS_IO_NONALERT, FILE_WRITE_THROUGH, FILE_OPEN_FOR_BACKUP_INTENT, and FILE_OPEN_BY_FILE_ID. The *CreateDisposition* parameter must be set to FILE_CREATE, FILE_OPEN, or FILE_OPEN_IF. |
| FILE_NON_DIRECTORY_FILE | The file is not a directory. The file object to open can represent a data file; a logical, virtual, or physical device; or a volume. |

















