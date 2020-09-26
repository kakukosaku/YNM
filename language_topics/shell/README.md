# Should be Familiar with Shell

## Overview

- [Tutorial](#tutorial)
- [Linux Command Examples](#linux-command-examples)

### Tutorial

Ref: https://www.tutorialspoint.com/unix/shell_scripting.htm

1. sh, bash, zsh, dash difference

https://stackoverflow.com/questions/5725296/difference-between-sh-and-bash

sh 是指 `shell script specification`, shell脚本的说明, bash, dash, zsh(debian, ubuntu等系统默认shell), 不同shell的实现, 会有细微差别;

`/bin/sh` 一变是具体shell实现的 symbolic link or hard link. 一般在shell脚本中指定 `#! /bin/sh` 即会自动选择系统指定的shell实现.

通过 `chsh`, change shell可以切换...

2. using shell variable

变量名, 仅可使用`a-zA-Z_` (-为正则表示法...), 一般为UPPER_CASE_FORMAT, e.g:

**scalar variables**

```bash
# Definition
NAME="kaku"
# NAME = "kaku" -> error!

# Access
echo $NAME

# Make var read-only
# readonly NAME

# Unset variable
unset NAME
echo ${NAME}  # echo empty string
echo ${NAME:-kaku}  # if NAME is unset or null echo kaku, but NAME still unset
echo ${NAME:=kaku}  # if NAME is unset or null echo kaku, and NAME is set to kaku
```

**special variables**

前面已经提到变量名只能是 `alphanumeric`, 这是因为 non-alphanumeric 有特殊的含义:

```bash
# Not all special variable, just list those Important
echo $0  # The filename of the current script.
ecoh $1  # $n, n从1起, 指script所接收的参数, 1 第一个参数, 以此类推
ecoh $#  # 指script所接收的参数的数量
echo $?  # The exist status of the last command executed.
echo $$  # current process id, PID
echo $!  # The PID of the last background command.
```

**Array**

```bash
# Definition
NAMES[0]="kaku"
NAMES[1]="kaku ko saku"

NAMES2=("kaku" "kaku ko saku")

# Access
echo ${NAMES2[1]}  # print `kaku ko saku`
echo ${NAMES2[*]}  # access all item of the array

# iterate array item:
for item in "${NAMES2[@]}"
do
    echo $item
done
```

3. Operators

```bash
# Arithmetic operations
VAL1=1+1
echo ${VAL1}  # print 1+1, not 2
VAL2=`expr 1 + 1`  # Import not `expr 1+1`
echo ${VAL2}  # print 2

# Compare operations
if [[ ${VAL1} == ${VAL2} ]]
then
    echo "equal var"
else
    echo "not equal val"
fi

# logic operations
if [[ !false ]]  # -o: or -a: and
then
    echo "always true"
fi

# file test operations
FILENAME="t.txt"
if test -d ${FILENAME}
then
    echo "directory"
elif test -f ${FILENAME}
then
    echo "file"
else
    echo "unk"
fi
```

4. flow control

**if ... fi**

```bash
if [[ !true ]]
then
    echo "never executed!"
elif [[ !false ]]
then
    echo "always true"
else
    echo "default if condition"
fi
```

**case...esac**

```bash
read val
case ${val} in
    1)
    echo "select 1"
    ;;
    2|3)
    echo "select 2 or 3"
    ;;
    *)
    echo "unk choice"
    ;;
esac
```

**loop**

```bash
# while loop
a=0
while [[ "$a" -lt 3 ]]    # this is loop1
do
   b="$a"
   while [[ "$b" -ge 0 ]]  # this is loop2
   do
      echo -n "$b "
      b=`expr ${b} - 1`
   done
   echo
   a=`expr ${a} + 1`
done
# Output:
# 0
# 1 0
# 2 1 0
# 如果在mac上输出不对, 将 #! /bin/sh 改为 #! /bin/zsh

# until loop
a=10
until [[ ${a} -lt 10 ]]
do
   echo -n "${a} "
   a=`expr ${a} + 1`
done
# Output:
# 10 9 8 7 6 5 4 3 2 1 0

# break and continue as other language! except `break n` or `continue n`: n specific times of break/continue
```

5. function

```bash
# define function
Hello() {
    echo "hello from function!" $1
    return 10
}

# call function
Hello

# call with argument
Hello kaku

# captcha function return value
echo $?
```

### Linux Command Examples

- sort, uniq

(shell/lines.txt 为示例数据)

单纯排序: `sort lines.txt` or `cat lines.txt | sort`

-r: 反序 reverse

-V: version 排序, 比如: `v1.2.0 < v1.10.0`, 否则, 按字串顺序排序

-u: 取唯一元素 unique

有时候, 可能需要`uniq` 来做更强大的处理如:

取唯一元素: `uniq lines.txt`

-d: 取重复出现的元素

-c: 将元素出现次数统计输出, 输出格式: `count element`

应用的场景:

1. 从日志中, 取出访问IP的 Top N. 结合lines.txt, 取出现次数最多的3个版本号: `cat lines.txt | sort -V | uniq -c | sort -r | head -n 3`

2. 2个文件做交并差, 如 a.txt 与 b.txt 

并集: `sort a.txt b.txt | uniq`

交集: `sort a.txt b.txt | uniq -d`

a-b的差集: `sort a.txt b.txt b.txt | uniq -u`

b-a的差集: `sort a.txt b.txt a.txt | uniq -u`

a-b差集解释: 添加2次b.txt, 再取unique元素使得结果集中必然没有b...即(a - b)
