#!/usr/bin/env python3

def find_memory_leaks(log_file):
    allocated = {}  # 记录内存块地址及其分配时的进程地址
    with open(log_file, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            parts = line.split(':')
            if len(parts) != 3:
                print(f"忽略无效行: {line}")
                continue
            op_type, proc_addr, mem_addr = parts
            if op_type == 'alloc':
                allocated[mem_addr] = proc_addr  # 记录分配操作
            elif op_type == 'free':
                if mem_addr in allocated:
                    del allocated[mem_addr]  # 释放则删除记录
            else:
                print(f"未知操作类型: {op_type}，行内容: {line}")

    # 输出泄漏结果
    if not allocated:
        print("未检测到内存泄漏")
    else:
        print("检测到以下内存泄漏:")
        for mem_addr, proc_addr in allocated.items():
            print(f"内存块地址: {mem_addr}, 分配位置: {proc_addr}")

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2:
        print("使用方法: ./check_mem.py malloc_free.log")
        sys.exit(1)
    find_memory_leaks(sys.argv[1])
