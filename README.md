# cloth_simulation_demo
用C++和opengl完成的 https://mp.weixin.qq.com/s/x6qX_SXCUdlTmGulx6okAg 的hard挑战^_^

本repo为一个visual studio工程，总计约800行C++代码。在个人的机器上，release模式执行可以达到60帧，而用taichi指定ti.init(arch=ti.cpu)，运行大约36帧，指定ti.init(arch=ti.cuda)运行可以达到117帧左右，指定ti.init(arch=ti.vulkan)甚至可以达到215帧。本仓库根目录下的cloth_simulation.py即为相应的taichi实现。

几处修改：

1、可以多个球，只需在main.cpp里改一下ball_number。默认是5个球。在shading模型中增加距离项，使得离光源更远的小球看上去更暗。

2、添加了布料和球的摩擦，但其实只是简单的将布料与小球相交处速度乘以一个比例系数fraction。

3、把重力和弹簧的弹力放到一个循环里面计算，而不是一开始先单独用重力更新一下速度。

4、把弹簧的劲度系数调小了一些。

5、其余参数与taichi原本的实现一致。然后参照learnopengl写了一个shading模型，不过似乎小球看上去跟taichi的ggui画出来的不太一样，推测是blinn-phong光照模型的一些参数引起的。

碰撞与布料的自相交等问题过于困难，所以并未实现。

# 环境与配置
visual studio 2019，同时需要在visual studio中自行配置opengl3.3(glfw3 & glad & glm0.9.9)，Eigen 3.3.9, 以及onetbb。可能还需要将visual studio设置为C++17版本。

机器配置情况：Intel i7-11800h， RTX 3060 Laptop GPU， 16GB内存（8GB*2）双通道， windows10

# 运行结果
![image](https://github.com/tlcui/cloth_simulation_demo/blob/master/results.gif)

支持WASD以及鼠标移动视角

# 其他
借助调用的各种库，此C++实现的帧数能超过ti.cpu（尽管远远不如ti.cuda和ti.vulkan）。我的程序里的并行计算几乎全是由tbb::parallel_for()完成的，同时CPU负载也明显比ti.cpu更高，运行一段时间后CPU温度达到93°C，而ti.cpu执行时CPU温度在83°C至88°C之间。

