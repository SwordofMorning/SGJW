# State Grid JPEG Wrapper

&emsp;&emsp;本仓库为兼容国家电网的JPEG的封装器。详细格式参考`ref/红外数据文件格式.docx`。

## 一、文件结构

```sh
.
├── build.sh                # 编译脚本
├── CMakeLists.txt          # 项目CMake
├── inc                     # SGJW源码
│   ├── sgjw.c
│   └── sgjw.h
├── pic                     # 测试图片
├── README.md               # Readme
└── src
    ├── CMakeLists.txt      # 测试程序CMake
    └── main.c              # 测试程序主函数
```

## 二、如何使用

1. 将`inc/`下的文件添加到项目中；
2. 用例参考`src/main.c`，或`inc/sgjw.h`中的文件描述。

## 三、注意事项

1. 本仓库**只负责**读取数据、追加(append)数据到JPEG中；
2. 本仓库**不包含**将数据以JPEG形式导出。