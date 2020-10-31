## 微信小程序端示例

1. 下载测试样例代码至本地

2. 使用微信开发者工具导入此项目, 填写自己的appid, 打开详情面板, **勾选不校验合法域名**

3. 打开**pages/wifi_station/tianqi/tianqi.js**, 将`devicesId`, `api_key`**替换为自己的**

   此处和上传时替换二者目的相同, 替换为你的设备ID, 才能把你的数据请求下来, apikey是你的才能允许请求, 避免他人恶意盗取信息

   ![](http://img.meansky.cn/mpOneNet/%E6%9B%BF%E6%8D%A2id.png)

4. 点击小程序按钮, 即可观察到实时温度湿度光照的数据, 图表会根据程序的设定自动更新

   ![](http://img.meansky.cn/mpOneNet/%E5%9B%BE%E8%A1%A8.png)