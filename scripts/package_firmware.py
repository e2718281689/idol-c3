import os
import json
import shutil
import sys
import argparse


def set_github_output(name, value):
    """设置 GitHub Actions 的输出变量"""
    # GITHUB_OUTPUT 环境变量包含了输出文件的路径
    github_output_file = os.getenv('GITHUB_OUTPUT')
    if github_output_file:
        with open(github_output_file, 'a') as f:
            # 格式是 'variable_name=value'
            f.write(f"{name}={value}\n")
    else:
        print(f"警告: GITHUB_OUTPUT 环境变量未设置，无法设置输出 {name}.")

def main():
    """
    根据 flasher_args.json 的内容打包固件文件，并保留其原始目录结构。
    """
    # 1. 设置命令行参数解析
    parser = argparse.ArgumentParser(description="Package firmware files based on flasher_args.json, preserving directory structure.")
    parser.add_argument("--build-dir", default="build", help="包含编译产物的目录")
    parser.add_argument("--output-dir", default="firmware_package", help="用于存放打包文件的临时目录")
    parser.add_argument("--zip-name-prefix", default="firmware", help="ZIP 文件名的前缀")
    parser.add_argument("--ref-name", required=True, help="Git 引用名 (例如 标签或分支名)")
    parser.add_argument("--target", required=True, help="构建的目标芯片 (例如 esp32c3)")
    
    args = parser.parse_args()

    # 2. 定义路径
    flasher_args_path = os.path.join(args.build_dir, "flasher_args.json")
    
    if not os.path.exists(flasher_args_path):
        print(f"错误: {flasher_args_path} 未找到！")
        sys.exit(1)
        
    # 3. 创建输出目录
    if os.path.exists(args.output_dir):
        shutil.rmtree(args.output_dir)
    os.makedirs(args.output_dir)

    print(f"从 {flasher_args_path} 读取文件列表...")

    # 4. 读取 JSON 文件并复制其中列出的文件
    try:
        with open(flasher_args_path, 'r') as f:
            data = json.load(f)
        
        files_to_copy = data.get("flash_files", {}).values()

        if not files_to_copy:
            print("警告: flasher_args.json 中没有找到 'flash_files' 条目或该条目为空。")
        
        for file_path in files_to_copy:
            source_file = os.path.join(args.build_dir, file_path)
            
            if os.path.exists(source_file):
                # ★★★ 核心改动在这里 ★★★
                # 构造完整的目标路径，保留原始子目录结构
                dest_file = os.path.join(args.output_dir, file_path)
                
                # 获取目标文件的目录路径
                dest_dir = os.path.dirname(dest_file)
                
                # 如果目标目录不存在，则创建它 (包括所有中间目录)
                os.makedirs(dest_dir, exist_ok=True)
                
                print(f"正在复制: {source_file} -> {dest_file}")
                shutil.copy(source_file, dest_file)
            else:
                print(f"警告: 文件 '{source_file}' 未找到，已跳过。")

        # 5. 复制 flasher_args.json 文件本身到输出目录的根
        print(f"正在复制 {flasher_args_path}...")
        shutil.copy(flasher_args_path, args.output_dir)

    except (json.JSONDecodeError, KeyError) as e:
        print(f"解析 {flasher_args_path} 时出错: {e}")
        sys.exit(1)

    # 6. 将输出目录打包成 ZIP 文件
    zip_filename_base = f"{args.zip_name_prefix}-{args.ref_name}-{args.target}"
    print(f"正在创建 ZIP 文件: {zip_filename_base}.zip...")
    
    shutil.make_archive(zip_filename_base, 'zip', args.output_dir)
    
    print("打包成功！")
    print(f"已生成: {zip_filename_base}.zip")

    zip_full_filename = f"{zip_filename_base}.zip"
    # 7. 将最终的文件名设置为 GitHub Actions 的输出
    set_github_output("zip_filename", zip_full_filename)

if __name__ == "__main__":
    main()

