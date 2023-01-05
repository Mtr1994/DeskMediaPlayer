# DeskMediaPlayer

![Platform](https://img.shields.io/badge/paltform-win10--64-brightgreen)
![Qt Version](https://img.shields.io/badge/_Qt_5.15.2-yellowgreen)
![Build](https://img.shields.io/badge/build-MSVC_2019_x64-blue)
![GitHub](https://img.shields.io/github/license/Mtr1994/DeskMediaPlayer)

  视频播放器程序，使用 FFMPEG 框架编写



#### 一、技术要点

​	1、使用 `OpenGL` 绘制图像，通过 `FFMPEG` 将所有图片格式转换为 `RGB24` 格式

​	2、添加了截图功能，能在不影响播放的情况下，在子线程中保存图片

​	3、添加了可靠的暂停、快进、快退、跳转逻辑

​	4、多级渐远纹理保证了窗口在小于视频尺寸时，纹理不会像素化严重

​	5、修改视频、音频等线程的循环逻辑，大幅降低 `CPU` 使用率

#### 二、更新日志

##### 2003年01月05日

- [x] 优化视频解析逻辑，通过固定帧数来进行通知，不必等待固定时长
- [x] 添加在暂停状态下的进度跳转功能
