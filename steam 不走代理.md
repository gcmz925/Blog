## steam 不走代理

有时候一些游戏，锁国区，挂上代理之后显示 当前地区不可用，原因是用了魔法。也有时候steam需要下载游戏时，也会走代理，会消耗套餐。当然最简单的方法是直接关闭魔法，可是在此期间就不能愉快的冲浪。

![image-20241221235210929](https://gcmz925-note.oss-cn-shenzhen.aliyuncs.com/gcmz/note202412212352175.png)

通过clash在进行登陆steam时可以查看连接日志观察访问了哪些网址（如果要屏蔽其它网页连接也可以这样查看），发现主要为如下两个连接

```javascript
*.cm.steampowered.com
*.steamserver.net
```

![image-20241221233511025](https://gcmz925-note.oss-cn-shenzhen.aliyuncs.com/gcmz/note202412212339579.png)

在 设置->系统代理->绕过的域/网络 中选择右侧的编辑

![image-20241221234336777](https://gcmz925-note.oss-cn-shenzhen.aliyuncs.com/gcmz/note202412212343836.png)

在配置文件中加入上述两条规则，进行绕过处理

![image-20241221234728502](https://gcmz925-note.oss-cn-shenzhen.aliyuncs.com/gcmz/note202412212347550.png)

点击右下角保存按钮，随后重启steam，锁国区的游戏可以使用

![image-20241221234855031](https://gcmz925-note.oss-cn-shenzhen.aliyuncs.com/gcmz/note202412212348289.png)



2.关于steam解决因为代理下载缓慢的问题可以参考以下链接

[Steam如何绕过clash的全局代理](https://cornradio.github.io/hugo/posts/%E8%AE%A9steam%E7%BB%95%E8%BF%87clash%E7%B3%BB%E7%BB%9F%E4%BB%A3%E7%90%86/)





3.配置文件

```shell
bypass:
# Steam中国大陆地区游戏下载
  - "steampipe.steamcontent.tnkjmec.com" #华为云
  - "st.dl.eccdnx.com" #白山云
  - "st.dl.bscstorage.net"
  - "st.dl.pinyuncloud.com"
  - "dl.steam.clngaa.com" #金山云
  - "cdn.mileweb.cs.steampowered.com.8686c.com" #网宿云
  - "cdn-ws.content.steamchina.com"
  - "cdn-qc.content.steamchina.com" #腾讯云
  - "cdn-ali.content.steamchina.com" #阿里云
# Steam非中国大陆地区游戏下载/社区实况直播
  - "*.steamcontent.com"
# Steam国际创意工坊下载CDN
  - "steamusercontent-a.akamaihd.net" #CDN-Akamai
# Origin游戏下载
  - "ssl-lvlt.cdn.ea.com" #CDN-Level3
  - "origin-a.akamaihd.net" #CDN-Akamai
# Battle.net战网中国大陆地区游戏下载
  - "client05.pdl.wow.battlenet.com.cn" #华为云
  - "client02.pdl.wow.battlenet.com.cn" #网宿云
# Battle.net战网非中国大陆地区游戏下载
  - "level3.blizzard.com" #CDN-Level3
  - "blzddist1-a.akamaihd.net" #CDN-Akamai
  - "blzddistkr1-a.akamaihd.net"
  - "kr.cdn.blizzard.com" #CDN-Blizzard
  - "us.cdn.blizzard.com"
  - "eu.cdn.blizzard.com"
# Epic Games中国大陆地区游戏下载
  - "epicgames-download1-1251447533.file.myqcloud.com"
# Epic Games非中国大陆地区游戏下载
  - "epicgames-download1.akamaized.net" #CDN-Akamai
  - "download.epicgames.com" #CDN-Amazon
  - "download2.epicgames.com"
  - "download3.epicgames.com"
  - "download4.epicgames.com"
# Rockstar Launcher客户端更新/游戏更新/游戏下载
  - "gamedownloads-rockstargames-com.akamaized.net"
# GOG中国大陆游戏下载/客户端更新
  - "gog.qtlglb.com"
# GOG非中国大陆游戏下载/客户端更新
  - "cdn.gog.com"
  - "galaxy-client-update.gog.com"
# steam游戏连接
  - "*.steamserver.net" 
  - "*.cm.steampowered.com"
# 其它自带
  - localhost
  - 127.*
  - 10.*
  - 172.16.*
  - 172.17.*
  - 172.18.*
  - 172.19.*
  - 172.20.*
  - 172.21.*
  - 172.22.*
  - 172.23.*
  - 172.24.*
  - 172.25.*
  - 172.26.*
  - 172.27.*
  - 172.28.*
  - 172.29.*
  - 172.30.*
  - 172.31.*
  - 192.168.*
  - <local>

```

