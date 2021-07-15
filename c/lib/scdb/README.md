# scdb

## Intro

[scdb] 是一个快速、可靠、简单的包，用于创建和读取常量数据库。它的数据库结构提供了几个特点：

- 快速查找：在大型数据库中成功查找通常只需要两次磁盘访问。不成功的查找只需要一个。
- 低开销：数据库使用 2048 字节，加上每条记录 24 字节，加上键和数据的空间。
- 没有随机限制： cdb 可以处理最大 4 GB 的任何数据库。没有其他限制；记录甚至不必放入内存。数据库以与机器无关的格式存储。
- 快速原子数据库替换： cdbmake 可以比其他散列包快两个数量级地重写整个数据库。
- 快速数据库转储： cdbdump 以与 cdbmake 兼容的格式打印数据库的内容。

> 其实，就是把 key=value， 这样的文件内容生成二进制 cdb 文件，用于提高查询速度。

## Usage

### 1. Generate CDB

#### Build

```bash
gcc -g -o scdb_make scdb_make.c buffer.c utils.c
```

#### Generate cdb with cfg file

cfg.ini:

```
version=1.0
name=cdb_test
email=kyosold@qq.com
```

```bash
./scdb_make 'cfg.ini' '=' 'cfg.cdb'
```

- scdb_make [cfg file] [strip char] [output cdb file]

### 2. Lookup CDB

#### Build

```bash
gcc -g -o scdb scdb.c buffer.c utils.c
```

#### Use

```bash
./scdb ./cfg.cdb 'email'
```

### 3. API

- use function `scdb_make_gen_file` generate cdb with file
- use function `scdb_get_alloc` lookup
