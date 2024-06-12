客户端：控制方	服务器：被控方

客户端：把数据打包放到线程中（SendPacket)，专门有个线程处理命令并将数据发送到服务端（threadFunc）

包格式：包头（FEFF）、包长度、包命令、包数据、和校验

效果图：

<img src="https://gitee.com/moshangzhishang/note-pic/raw/master/img/20240612095647.png" alt="QQ截图20240612095434" style="zoom:50%;" />

<img src="https://gitee.com/moshangzhishang/note-pic/raw/master/img/20240612095729.png" alt="QQ截图20240612095434" style="zoom:50%;" />