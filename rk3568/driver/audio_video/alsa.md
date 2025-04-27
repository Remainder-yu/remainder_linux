# alsa声卡设备
## PCM
PCM 脉冲编码调制
音频驱动的核心任务：
1. playback如何把用户空间的应用程序发过来的PCM数据，转化为人耳可以辨别的模拟音频
2. capture 把mic拾取得到模拟信号，经过采样、量化、转换为PCM信号送回给用户空间的应用程序
ASOC基本上将嵌入式音频系统拆分为多个可重复使用的组件驱动程序：
1. Codec class drivers： codec class driver与平台无关，包含音频控件、音频接口功能、编解码DAPM和编解码器IO函数。是可以在任何体系结构和机器上运行的通用代码
2. Platform class drivers：平台类驱动程序包括音频DMA引擎驱动程序、数字音频接口DAI，驱动程序（I2S.PCM）以及该平台的任何音频DSP驱动程序
3. Machine class driver：机器驱动程序类充当粘合剂，描述并将其组件驱动程序绑定在一起形成ALSA声卡设备。它处理任何机器特定的控制和机器级音频事件
## ASOC硬件架构
所以对应不同的codec？A1有哪些codec流通路线：
Codec功能主要以下4种：
(1)对PCM等信号进行D/A转换，把数字的音频信号转换为模拟信号
(2)对Mic、Linein或者其他输入源的模拟信号进行A/D转换，把模拟的声音信号转变CPU能够处理的数字信号
(3)对音频通路进行控制，比如播放音乐，收听调频收音机，又或者接听电话时，音频信号在codec内的流通路线是不一样的
(4)对音频信号做出相应的处理，例如音量控制，功率放大，EQ控制等等
### ASOC软件架构
在软件层面，ASOC也把嵌入式设备的音频系统同样分为3大部分，Machine\Platform\Codec。Codec驱动Asoc中的一个重要设计原则就是要求codec驱动是平台无关的，它包含了一些音频的控件（Controls），音频接口，DAPM（动态音频电源管理）的定义和某些codec IO功能。为了保证硬件无关性，任何特定于平台和机器的代码都要移到platform和machine驱动中。Codec驱动提供以下特性：
1. Codec DAI和PCM的配置信息
2. Codec的IO控制方式（I2C,SPI等）
3. Mixer和其他的音频控件
4. Codec的ASLA音频操作接口
必要时，提供以下功能：
1. DAPM描述信息
2. DAPM事件处理程序
3. DAC数字静音控制
4.
#### Machine部分
```cpp
snd_soc_dai_link：音频链路描述及板级操作函数，指定了Platform、Codec、codec_dai、cpu_dai的名字，Machine利用这些名字去匹配系统中注册的platform，codec，dai模块。snd_soc_register_card：注册platform_driver时触发prob函数，其中调用 snd_soc_register_card 注册Machine驱动，它正是整个ASoC驱动初始化的入口。
soc_bind_dai_link：扫描三个全局的链表头变量：codec_list、dai_list、platform_list，根据card->dai_link[]中的名称进行匹配。
```
####  Platform驱动
snd_soc_platform_driver：负责管理音频数据，简单的说就是对音频DMA的设置。name要和前面snd_soc_dai_link 结构中定义的相同，snd_soc_platform_driver才会被关联。snd_soc_dai_driver：主要完成cpu一侧的dai的参数配置，也就是对cpu端音频控制器的寄存器的设置，例如时钟频率、采样率、数据格式等等的设置。
struct snd_pcm_ops ：
```cpp
.open ：打开设备，准备开始播放的时候调用，这个函数主要是调用snd_soc_set_runtime_hwparams设置支持的音频参数。snd_dmaengine_pcm_open打开DMA引擎。
.close：关闭播放设备的时候回调。该函数负责关闭DMA引擎。释放相关的资源。
.ioctl：应用层调用的ioctl会调用这个回调。
.hw_params：在open后，应用设置播放参数的时候调用，根据设置的参数，设置DMA，例如数据宽度，传输块大小，DMA地址等。
.hw_free :  关闭设备前被调用，释放缓冲。
.trigger:  DAM开始时传输，结束传输，暂停传世，恢复传输的时候被回调。
.pointer： 返回DMA缓冲的当前指针。
.mmap ：   建立内存映射。
```


#### codec音频操作接口
codec音频接口分为5大部分：时钟配置、格式配置、数字静音、PCM音频接口、FIFO延迟。
```cpp
static const struct snd_soc_dai_ops rk817_dai_ops = {
    .hw_params  = rk817_hw_params,
    .set_fmt    = rk817_set_dai_fmt,
    .set_sysclk = rk817_set_dai_sysclk,
    .mute_stream    = rk817_digital_mute,
    .no_capture_mute    = 1,
};
```
snd_soc_codec_driver：音频编解码芯片描述及操作函数，如控件，音频路由的描述信息，时钟配置，IO控制等。
问题：怎么完成snd_soc_dai_driver的注册？
为什么需要添加control才能codec工作，添加control的方法是怎样的？

### 播放和采集音频数据涉及多个硬件设备
* CPU内存通过I2S和Audio Codec传递音频PCM数据
* CPU内存通过I2C总线和Audio codec传递控制信息，如静音、音量等
* Audio codec执行数字音频数据和模拟音频信号的转换等




[alsa](https://www.zhihu.com/search?type=content&q=alsa)