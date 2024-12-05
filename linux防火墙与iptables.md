## linux防火墙与iptables

## linux防火墙

在 [CentOS](https://www.linuxidc.com/topicnews.aspx?tid=14) 7 中，引入了一个新的服务，Firewalld，下面一张图，让大家明确的了解防火墙 Firewall 与 iptables 之间的关系与区别。

![img](https://i-blog.csdnimg.cn/blog_migrate/dda2bff5d6a56dcad272d8b45e6fbbf4.png)





### firewalld

> 防火墙守护 firewalld 服务引入了一个信任级别的概念来管理与之相关联的连接与接口。它支持 ipv4 与 ipv6，并支持网桥，采用 firewall-cmd (command) 或 firewall-config (gui) 来动态的管理 kernel netfilter 的临时或永久的接口规则，并实时生效而无需重启服务。
>
> 
>
> zone
>
> - drop: 丢弃所有进入的包，而不给出任何响应
> - block: 拒绝所有外部发起的连接，允许内部发起的连接
> - public: 允许指定的进入连接
> - external: 同上，对伪装的进入连接，一般用于路由转发
> - dmz: 允许受限制的进入连接
> - work: 允许受信任的计算机被限制的进入连接，类似 workgroup
> - home: 同上，类似 homegroup
> - internal: 同上，范围针对所有互联网用户
> - trusted: 信任所有连接
>
> 过滤规则
>
> - source: 根据源地址过滤
> - interface: 根据网卡过滤
> - service: 根据服务名过滤
> - port: 根据端口过滤
> - icmp-block: icmp 报文过滤，按照 icmp 类型配置
> - masquerade: ip 地址伪装
> - forward-port: 端口转发
> - rule: 自定义规则
>
> 其中，过滤规则的优先级遵循如下顺序
>
> 1.source
> 2.interface
> 3.firewalld.conf







使用方法:



#### 关闭防火墙：

```
systemctl stop firewalld.service
```
#### 开启防火墙：

```
systemctl start firewalld.service
```
若遇到无法开启
```
先用：systemctl unmask firewalld.service 
然后：systemctl start firewalld.service
```
#### 开启开机启动：

```
systemctl enable firewalld.service
```
#### 关闭开机启动：

```
systemctl disable firewalld.service
```
#### 查看防火墙状态：

```
systemctl status firewalld 
firewall-cmd --state
```

#### 开启端口

```
#（--permanent永久生效，没有此参数重启后失效）
#注：可以是一个端口范围，如1000-2000/tcp
firewall-cmd --zone=public --add-port=80/tcp --permanent    
```
#### 重新载入

```
firewall-cmd --reload
```
#### 查询某个端口是否开放

```
firewall-cmd --query-port=80/tcp
```
#### 移除端口

```
firewall-cmd --zone=public --remove-port=80/tcp --permanent
firewall-cmd --permanent --remove-port=123/tcp
```
#### 查询已经开放的端口列表

```
firewall-cmd --list-port

命令含义：

--zone #作用域

--add-port=80/tcp #添加端口，格式为：端口/通讯协议

--remove-port=80/tcp #移除端口，格式为：端口/通讯协议

--permanent #永久生效，没有此参数重启后失效
```

#### 配置firewalld-cmd

```
查看版本： firewall-cmd --version

查看帮助： firewall-cmd --help

显示状态： firewall-cmd --state

查看所有打开的端口： firewall-cmd --zone=public --list-ports

更新防火墙规则： firewall-cmd --reload

查看区域信息:  firewall-cmd --get-active-zones

查看指定接口所属区域： firewall-cmd --get-zone-of-interface=eth0

拒绝所有包：firewall-cmd --panic-on

取消拒绝状态： firewall-cmd --panic-off

查看是否拒绝： firewall-cmd --query-panic
```








### iptables

> [-t 表名]：该规则所操作的哪个表，可以使用filter、nat等，如果没有指定则默认为filter
> -A：新增一条规则，到该规则链列表的最后一行
> -I：插入一条规则，原本该位置上的规则会往后顺序移动，没有指定编号则为1
> -D：从规则链中删除一条规则，要么输入完整的规则，或者指定规则编号加以删除
> -R：替换某条规则，规则替换不会改变顺序，而且必须指定编号。
> -P：设置某条规则链的默认动作
> -nL：-L、-n，查看当前运行的防火墙规则列表
> chain名：指定规则表的哪个链，如INPUT、OUPUT、FORWARD、PREROUTING等
> [规则编号]：插入、删除、替换规则时用，`--line-numbers`显示号码
> [-i|o 网卡名称]：i是指定数据包从哪块网卡进入，o是指定数据包从哪块网卡输出
> [-p 协议类型]：可以指定规则应用的协议，包含tcp、udp和icmp等
> [-s 源IP地址]：源主机的IP地址或子网地址
> [–sport 源端口号]：数据包的IP的源端口号
> [-d目标IP地址]：目标主机的IP地址或子网地址
> [–dport目标端口号]：数据包的IP的目标端口号
> -m：extend matches，这个选项用于提供更多的匹配参数，如：
> -m state –state ESTABLISHED,RELATED
> -m tcp –dport 22
> -m multiport –dports 80,8080
> -m icmp –icmp-type 8
> <-j 动作>：处理数据包的动作，包括ACCEPT、DROP、REJECT等





#### 查看规则

查看规则集

```bash
# iptables --list -n
```

加一个-n以数字形式显示IP和端口，而不是默认的服务名称

#### 修改规则

配置默认规则，不允许数据进入

```bash
# iptables -P INPUT DROP
```

允许转发

```bash
# iptables -P FORWARD ACCEPT
```

不允许数据流出

```bash
# iptables -P OUTPUT DROP
```

#### 添加规则

允许源IP地址为192.168.0.0/24网段的包流进（包括所有的协议，这里也可以指定单个IP）

```bash
iptables -A INPUT -s 192.168.0.0/24 -j ACCEPT
iptables -A INPUT -s 192.168.0.0/24 -j DROP
```

允许所有的IP到192.168.0.22的访问

```bash
iptables -A INPUT -d 192.168.0.22 -j ACCEPT
iptables -A INPUT -d 192.168.0.22 -j DROP
```

开放本机80端口

```bash
iptables -A INPUT -p tcp --dport 80 -j ACCEPT
iptables -A INPUT -p tcp --dport 80 -j DROP
```

开放本机的ICMP协议

```bash
iptables -A INPUT -p icmp --icmp-type echo-request -j ACCEPT
```

#### 删除规则

删除允许源地址进入的规则

```bash
iptables -D INPUT -s 192.168.0.21 -j ACCEPT
```

#### iptables服务命令

启动服务

```bash
/etc/init.d/iptables start

# 或

service iptables start
```



停止服务

```bash
/etc/init.d/iptables stop

# 或者

service iptables stop
```



重启服务

```bash
/etc/init.d/iptables restart

# 或者

service iptables restart
```

#### 空当前的所有规则和计数

清空所有的防火墙规则

```bash
iptables -F
```

删除用户自定义的空链

```bash
iptables -X
```

清空计数

```bash
iptables -Z
```

清空指定链 INPUT 上面的所有规则

```bash
iptables -F INPUT
```

删除指定的链，这个链必须没有被其它任何规则引用，而且这条上必须没有任何规则。

```bash
iptables -X INPUT
```

如果没有指定链名，则会删除该表中所有非内置的链。

把指定链，或者表中的所有链上的所有计数器清零。

```bash
iptables -Z INPUT
```

#### 保存规则

保存设置,将规则保存在/etc/sysconfig/iptables文件里

##### 方法1:

```bash
/etc/init.d/iptables save

# 或者

service iptables save
```



> iptables防火墙的配置文件存放于：/etc/sysconfig/iptables

##### 方法2:

修改/etc/sysconfig/iptables-config 将里面的IPTABLES_SAVE_ON_STOP=”no”, 这一句的”no”改为”yes”这样每次服务在停止之前会自动将现有的规则保存在 /etc/sysconfig/iptables 这个文件中去。

### UFW

#### Ubuntu安装UFW防火墙

```
sudo apt-get install ufw 
```

一般用户，只需如下设置：
```
sudo apt-get install ufw
sudo ufw enable
sudo ufw default deny
```
以上三条命令已经足够安全了，如果你需要开放某些服务，再使用sudo ufw allow开启。

开启防火墙
```shell
sudo ufw enable 
sudo ufw default deny 
#运行以上两条命令后，开启了防火墙，并在系统启动时自动开启。
#关闭所有外部对本机的访问，但本机访问外部正常。
```



#### 开启/禁用

```shell
sudo ufw allow|deny [service] 
```

#### 打开或关闭端口

```shell
sudo ufw allow smtp　      #允许所有的外部IP访问本机的25/tcp (smtp)端口 
sudo ufw allow 22/tcp      #允许所有的外部IP访问本机的22/tcp (ssh)端口 
sudo ufw allow 53          #允许外部访问53端口(tcp/udp) 
sudo ufw allow from 192.168.1.100 #允许此IP访问所有的本机端口 
sudo ufw allow proto udp 192.168.0.1 port 53 to 192.168.0.2 port 53 
sudo ufw deny smtp         #禁止外部访问smtp服务 
sudo ufw delete allow smtp #删除上面建立的某条规则 
```

#### 查看防火墙状态

```shell
sudo ufw status 
```

补充：开启/关闭防火墙 (默认设置是’disable’)
```shell
ufw enable|disable
```

转换日志状态
```shell
ufw logging on|off
```

设置默认策略 (比如 “mostly open” vs “mostly closed”)

```shell
ufw default allow|deny

```

#许可或者屏蔽某些入埠的包 (可以在“status” 中查看到服务列表［见后文］)
#可以用“协议：端口”的方式指定一个存在于/etc/services中的服务名称，也可以通过包的meta-data。 ‘allow’ 参数将把条目加入 /etc/ufw/maps ，而 ‘deny’ 则相反。基本语法如下：ufw allow|deny [service]
#显示防火墙和端口的侦听状态，参见 /var/lib/ufw/maps。括号中的数字将不会被显示出来。

```shell
ufw status
```



#### UFW使用范例：

```shell
#允许 53 端口
$ sudo ufw allow 53

#禁用 53 端口
$ sudo ufw delete allow 53

#允许 80 端口
$ sudo ufw allow 80/tcp

#禁用 80 端口
$ sudo ufw delete allow 80/tcp

#允许 smtp 端口
$ sudo ufw allow smtp

#删除 smtp 端口的许可
$ sudo ufw delete allow smtp

#允许某特定 IP
$ sudo ufw allow from 192.168.254.254

#删除上面的规则
$ sudo ufw delete allow from 192.168.254.254    
```



### systemctl

systemctl是CentOS7的服务管理工具中主要的工具，它融合之前service和chkconfig的功能于一体。

```
启动一个服务：systemctl start firewalld.service
关闭一个服务：systemctl stop firewalld.service
重启一个服务：systemctl restart firewalld.service
显示一个服务的状态：systemctl status firewalld.service
在开机时启用一个服务：systemctl enable firewalld.service
在开机时禁用一个服务：systemctl disable firewalld.service
查看服务是否开机启动：systemctl is-enabled firewalld.service
查看已启动的服务列表：systemctl list-unit-files|grep enabled
查看启动失败的服务列表：systemctl --failed
```
