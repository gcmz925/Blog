## 挂起进程

在 Linux 系统中，可以使用 `kill` 命令结合信号来挂起某个进程。要挂起某个 PID，可以发送 `SIGSTOP` 信号：

```bash
kill -SIGSTOP <PID>
```

替换 `<PID>` 为你要挂起的进程的实际进程 ID。要恢复进程，可以发送 `SIGCONT` 信号：

```bash
kill -SIGCONT <PID>
```



## **Btrfs** 文件系统

**Btrfs** 是一种先进的文件系统，支持快照、子卷和多挂载等特性。它通过将数据和元数据分开存储，提高了性能和可靠性。以下是一些主要功能和使用方法：

### 主要特性
1. **快照**：可以在任意时刻创建文件系统的只读快照，方便备份和恢复。
2. **子卷**：可以创建多个子卷，每个子卷都像独立的文件系统，支持不同的挂载选项。
3. **多挂载**：同一子卷可以在不同的挂载点同时挂载，允许灵活的数据共享。
4. **在线压缩**：支持在数据写入时进行压缩，以节省存储空间。

### 挂载 Btrfs
1. **格式化**：
   ```bash
   mkfs.btrfs /dev/sdX
   ```
   替换 `/dev/sdX` 为你的设备。

2. **挂载**：
   ```bash
   mount -t btrfs /dev/sdX /mnt
   ```

3. **创建子卷**：
   ```bash
   btrfs subvolume create /mnt/my_subvolume
   ```

4. **挂载子卷**：
   ```bash
   mount -o subvol=my_subvolume /dev/sdX /mnt/my_mount_point
   ```

5. **使用快照**：
   
   ```bash
   btrfs subvolume snapshot /mnt/my_subvolume /mnt/my_snapshot
   ```

### 多挂载
要多次挂载同一个子卷，可以使用 `mount` 命令多次指定不同的挂载点。例如：
```bash
mount -o subvol=my_subvolume /dev/sdX /mnt/point1
mount -o subvol=my_subvolume /dev/sdX /mnt/point2
```

这样，你可以在 `/mnt/point1` 和 `/mnt/point2` 访问同一个子卷的内容。







