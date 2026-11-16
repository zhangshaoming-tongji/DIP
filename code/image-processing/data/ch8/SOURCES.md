# 第 8 章样例图像来源说明

课堂实验直接使用下表中的图像，不附带下载缓存。

| 文件 | 用途 | 来源 |
| --- | --- | --- |
| `lena.png` | 例2、例3 | 课程标准测试图 |
| `cameraman.png` | 例2 相位交换 | 课程标准测试图 |
| `baboon.png` | 可选彩色纹理练习 | [OpenCV samples](https://github.com/opencv/opencv/blob/4.x/samples/data/baboon.jpg) |
| `pan.png` | 例4 山地全色，**512×512** | [PanScale / flandsat](https://huggingface.co/datasets/kecao/PanScale) `test800/pan/1.tif` 中心裁切，CC BY 4.0 |
| `ms_color.png` | 例4 山地多光谱真彩色，**256×256** | 同景 `test800/ms/1.tif` 对应局部（全色的 1/2），取前三波段 |
| `rs_reference.png` | 山地彩色图放大后的对照 | `ms_color.png` 双线性放大到 512×512 |
| `pan_town.png` | 例4 村镇全色，**512×512** | [LandCover.ai](https://landcover.ai.linuxpolska.com/) 正射影像 `N-33-119-C-c-3-3.tif` 中一块含房屋、道路和林地的 512×512 区域，转成灰度，像素未再缩放 |
| `ms_town.png` | 例4 村镇彩色，**256×256** | 同一块彩色区域在长和宽上各取 2×2 像素平均，缩小为二分之一 |

村镇一组来自波兰航空正射影像，地面分辨率约为 0.5 米，许可协议为 CC BY-NC-SA 4.0。全色图保留裁切范围内的原始像素，所以屋顶、小路和树冠的边缘仍然清楚；彩色图才被缩小。引用请见 Boguszewski 等，CVPR Workshops，2021。
