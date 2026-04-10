import re

def convert_sty_to_js(sty_file_path, js_file_path):
    try:
        with open(sty_file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except FileNotFoundError:
        print(f"错误：找不到文件 {sty_file_path}")
        return

    # 正则解析逻辑
    pattern = re.compile(
        r'\\NewDocumentCommand\s+(\\U\w+)\s*\{o\}\s*\{'
        r'\\str_if_eq:nnTF\{#1\}\{a\}\{\}\{(.*?)\}'
        r'(.*)\}'
        r'\s*%?(.*?)$',
        re.MULTILINE
    )

    commands = []

    matches = pattern.finditer(content)

    for match in matches:
        cmd_name_raw = match.group(1)
        spacing_part = match.group(2).strip()
        unit_part = match.group(3).strip()

        cmd_name = cmd_name_raw.replace('\\', '')

        default_def = spacing_part + unit_part
        a_def = unit_part

        commands.append({
            'name': cmd_name,
            'default': default_def,
            'a': a_def
        })

    if not commands:
        print("警告：未找到符合条件的命令定义。")
        return

    # 按长度降序排序，防止短命令破坏长命令
    commands.sort(key=lambda x: len(x['name']), reverse=True)

    # ---------------- 生成 JS 文件内容 ----------------

    js_lines = []
    js_lines.append("/* eslint-disable no-undef */")
    js_lines.append("")
    js_lines.append("/****************************************************")
    js_lines.append(" *  PhyUnit.js")
    js_lines.append(" *  自动生成的物理单位 Temml 宏定义")
    js_lines.append(" ****************************************************/")
    js_lines.append("")

    # 1. 生成预处理函数
    js_lines.append("function preprocessLatexUnits(latex) {")

    cmd_list_str = ", ".join([f"'{c['name']}'" for c in commands])
    js_lines.append(f"    const unitCommands = [{cmd_list_str}];")
    js_lines.append("")
    js_lines.append("    unitCommands.forEach(cmd => {")
    js_lines.append("        // 步骤1: 替换 \\cmd[a] 为 \\cmd@a")
    js_lines.append("        latex = latex.replace(new RegExp(`\\\\\\\\${cmd}\\\\s*\\\\[a\\\\]`, 'g'), `\\\\${cmd}@a`);")
    js_lines.append("")
    js_lines.append("        // 步骤2: 替换 \\cmd[其他] 为 \\cmd@default")
    js_lines.append("        latex = latex.replace(new RegExp(`\\\\\\\\${cmd}\\\\s*\\\\[[^\\\\]]*\\\\]`, 'g'), `\\\\${cmd}@default`);")
    js_lines.append("")
    js_lines.append("        // 步骤3: 替换裸命令 \\cmd 为 \\cmd@default")
    js_lines.append("        latex = latex.replace(new RegExp(`\\\\\\\\${cmd}(?![a-zA-Z]|@)`, 'g'), `\\\\${cmd}@default`);")
    js_lines.append("    });")
    js_lines.append("")
    js_lines.append("    return latex;")
    js_lines.append("}")
    js_lines.append("")

    # 2. 生成宏定义
    js_lines.append("/****************************************************")
    js_lines.append(" *  宏定义")
    js_lines.append(" ****************************************************/")

    for cmd in commands:
        name = cmd['name']

        # === 关键修复点 ===
        # 必须将定义内容中的反斜杠 \ 替换为 \\
        # 这样写入 JS 文件时才是 \"\\Um...\"，JS 运行时解析为 "\Um..."

        def_default_escaped = cmd['default'].replace('\\', '\\\\')
        def_a_escaped = cmd['a'].replace('\\', '\\\\')

        # === 关键修复点 ===
        # 宏名称前必须是双反斜杠 \\\\
        # 结果：temml.__defineMacro("\\Um@default", ...)
        js_lines.append(f"temml.__defineMacro(\"\\\\{name}@default\", \"{{{def_default_escaped}}}\");")
        js_lines.append(f"temml.__defineMacro(\"\\\\{name}@a\", \"{{{def_a_escaped}}}\");")

    # 3. 导出
    js_lines.append("")
    js_lines.append("temml.preprocessLatexUnits = preprocessLatexUnits;")

    try:
        with open(js_file_path, 'w', encoding='utf-8') as f:
            f.write('\n'.join(js_lines))
        print(f"成功转换 {len(commands)} 个命令！")
        print(f"已生成: {js_file_path}")
    except IOError as e:
        print(f"写入错误: {e}")

if __name__ == "__main__":
    convert_sty_to_js("PhyUnit.sty", "PhyUnit.js")
