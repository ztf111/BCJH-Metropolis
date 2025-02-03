# 基于模拟退火的计算器

![](https://img.shields.io/github/downloads/hjenryin/BCJH-Metropolis/total)
![](https://img.shields.io/github/stars/hjenryin/BCJH-Metropolis)

## 在线运行

访问[run.bcjh.xyz](run.bcjh.xyz)。参见[此仓库](https://github.com/yuwenxifan/BCJH-Metropolis-html)。

## 使用方法（本地安装包）

前往[Releases · hjenryin/BCJH-Metropolis](https://github.com/hjenryin/BCJH-Metropolis/releases)下载最新 `bcjh.zip`，解压后输入游戏内校验码即可使用。

### 配置厨具

 `厨具样例.csv`已在2.0.0弃用。直接使用白采菊花代码，保留高级厨具，新手池厨具可变。

### 更改迭代次数

如有需要，可以更改 `点此运行.bat` 中39行的数字来设置迭代次数。

## 本地编译

如果在windows平台，需要使用MSBuild编译，并将 `data/data.min.json` 改为ANSI编码。如有条件，强烈建议这种本地运行（运行可加速、可多次重复）。

- 在 `./data` 下运行 `python update.py --BCJH xxxx`，`xxxx`是游戏中的白菜菊花校验码。
- 编译、运行：

  - 在根目录新建 `build` 文件夹并 `cd` 至build
  - ```
    # linux
    cmake .. --config Release 
    make

    # windows (Visual Studio取决于安装的版本)
    cmake .. -G "Visual Studio 17 2022" --config Release
    msbuild /m
    ```
  - 使用 `./bcjh` (linux) 或 `./bcjh.exe` (windows) 运行。
- 最后在命令行就可以得到输出结果了！（注意顺序）（得到厨师-技法，表示对应新手池厨具）
- 注：linux下不支持多任务选择

## Github云端编译已在2.0.0弃用。

## 局限性

- 已知的问题：

  - **无法得到最优解！只能得到一个比较好的解，有助于开阔思路。**
  - 无法选择菜品数量（默认拉满）。
- 可能有一定门槛。（这可能有助于防止这类辅助工具的滥用导致分数膨胀？）（你问我为什么不用其他语言写？python一个晚上就写好了，结果因为有涉及json读写很多类型没法推断，jit用不了，算这个太慢了，所以就用c++写了）

## 工作原理

采用两层模拟退火来最大化总能量。第一层为三个厨师，其能量用第二层模拟退火来估计。也就是说，这套方法理论上也能算厨神（只要能够在非常快的时间内，算出一个厨神面板的得分），但是加上厨神的食材限制工作量有点大……以后再说吧。（希望以后的宴会不要有菜品限制）

### 如果你也想为此仓库添砖加瓦……

- c++实现厨神算分
- 以及任何你觉得有意义的事！

### 最后，欢迎大家提pr！有什么问题也可以在issue里面讨论！

这算是我写的第一个项目，本人本科在读，水平有限，写的不好的地方也请大佬们斧正！
