# 自动测试框架

## 环境准备

1. 安装python3
2. 安装依赖

```bash
pip install -r requirements.txt -i https://pypi.tuna.tsinghua.edu.cn/simple
```

## 开始测试

### 全量测试

```bash
cd testcases
python3 autotest.py -d . -n ../cmake-build-debug/rich.exe
```

参数解释：

 - -d 测试文件.in .out 所在文件夹
 - -n 可执行文件

### 模块测试

```bash
cd testcases/toolhouse
python3 ../autotest.py -d . -n ../../cmake-build-debug/rich.exe
```

### 将测试用例加入全量测试的方式

编辑testcases/config.json

在import中加入模块测试用例所在的文件夹

## 查看结果

结果会生成在当前目录的report.csv中，全量测试会分模块展示，注意进行测试的时候，不要用excel等软件打开report.csv，此类软件会锁住report.csv导致自动测试脚本无法重新写入（可以用vs code开report.csv 无影响）

## 测试规范

这里主要介绍测试结果为False时的处理措施

- 查看改用例所用的测试时间，若为5s（现已改为10s，因为the_god_of_wealth_2的用例时间为5.2s），检查用例超时问题，尝试减短用例所用时间，实在不行再尝试修改自动测试框架限制时间。自动控制框架限制时间是为了在程序长时间不输出时kill掉该线程
- 检查.in与.out文件正确性，检查是否因为命令丢失导致超时（一般测试用例时间达到临界多半是这种问题）
- 汇报bug，联系开发人员进行修改
- 若不确定需求，联系各组测试人员或者让PO去需求澄清