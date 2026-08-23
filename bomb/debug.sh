#!/usr/bin/env bash

set -euo pipefail

CONTAINER_NAME="csapp-env"
CONTAINER_BOMB_DIR="/workspace/bomb"
DEBUG_PORT="1234"

usage() {
    cat <<'EOF'
Bomb Lab 调试助手

用法：
  ./debug.sh start              启动 Docker 容器
  ./debug.sh bomb               启动 Bomb；断点就绪后自动输入 answers.txt
  ./debug.sh run                使用 answers.txt 正常运行 Bomb
  ./debug.sh gdb [函数名]       连接 GDB，并自动断在该函数
  ./debug.sh asm 函数名         查看汇编，每行标注“函数名+偏移”
  ./debug.sh addr 函数名        查询函数的真实地址
  ./debug.sh functions [关键字] 列出 Bomb 中的函数
  ./debug.sh reset              重启容器，清理残留 Bomb/GDB 进程
  ./debug.sh status             查看容器和调试进程状态
  ./debug.sh help               显示本帮助

推荐流程：
  终端 1：./debug.sh bomb
  终端 2：./debug.sh gdb phase_2

连接后：先 continue；终端 1 显示“等待 GDB 放行答案”后，在 GDB 按 Ctrl-C；
再输入 bomb-arm 和 continue。answers.txt 会自动输入。
EOF
}

require_docker() {
    if ! command -v docker >/dev/null 2>&1; then
        echo "错误：宿主机找不到 docker 命令。" >&2
        exit 1
    fi
}

container_exists() {
    docker inspect "$CONTAINER_NAME" >/dev/null 2>&1
}

ensure_container_running() {
    require_docker

    if ! container_exists; then
        echo "错误：找不到容器 ${CONTAINER_NAME}。" >&2
        exit 1
    fi

    if [[ "$(docker inspect --format '{{.State.Running}}' "$CONTAINER_NAME")" != "true" ]]; then
        echo "正在启动容器 $CONTAINER_NAME..."
        docker start "$CONTAINER_NAME" >/dev/null
    fi
}

validate_phase() {
    local function_name="$1"

    if [[ ! "$function_name" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]]; then
        echo "错误：函数名格式不正确：$function_name" >&2
        exit 1
    fi
}

symbol_range() {
    local function_name="$1"

    docker exec --workdir "$CONTAINER_BOMB_DIR" "$CONTAINER_NAME" \
        sh -c 'nm -n --defined-only ./bomb | awk -v wanted="$1" '\''
            $3 == wanted { print "0x" $1; found = 1; next }
            found && ($2 == "T" || $2 == "t") { print "0x" $1; exit }
        '\''' sh "$function_name"
}

symbol_address() {
    symbol_range "$1" | sed -n '1p'
}

require_function() {
    local function_name="$1"
    local function_addr

    validate_phase "$function_name"
    function_addr="$(symbol_address "$function_name")"
    if [[ -z "$function_addr" ]]; then
        echo "错误：Bomb 中找不到函数 ${function_name}。" >&2
        echo "可以运行 ./debug.sh functions 查看函数名。" >&2
        exit 1
    fi
}

command_name="${1:-help}"

case "$command_name" in
    start)
        ensure_container_running
        echo "容器 ${CONTAINER_NAME} 已运行。"
        echo "下一步：在终端 1 执行 ./debug.sh bomb"
        ;;

    bomb)
        ensure_container_running
        echo "正在启动 Bomb。它会先安静地等待 GDB 连接，这是正常现象。"
        echo "现在请在终端 2 执行：./debug.sh gdb phase_N"
        echo "断点设置完成后，answers.txt 会自动送入 Bomb。"
        echo
        docker exec -it \
            --workdir "$CONTAINER_BOMB_DIR" \
            --env "BOMB_DEBUG_PORT=$DEBUG_PORT" \
            "$CONTAINER_NAME" \
            bash -lc '
                answer_fifo=/tmp/csapp-bomb-answers.fifo
                ready_file=/tmp/csapp-bomb-gdb-ready
                rm -f "$answer_fifo" "$ready_file"
                mkfifo "$answer_fifo"
                (
                    echo "Bomb 已加载后会等待；请在 GDB 按 Ctrl-C 并执行 bomb-arm。"
                    while [[ ! -e "$ready_file" ]]; do sleep 0.1; done
                    if [[ -s answers.txt ]]; then
                        cat answers.txt > "$answer_fifo"
                    else
                        echo "警告：answers.txt 为空，将切换为人工输入。" >&2
                        : > "$answer_fifo"
                    fi
                ) &
                ROSETTA_DEBUGSERVER_PORT="$BOMB_DEBUG_PORT" exec ./bomb "$answer_fifo"
            '
        ;;

    run)
        ensure_container_running
        docker exec -it \
            --workdir "$CONTAINER_BOMB_DIR" \
            "$CONTAINER_NAME" \
            bash -lc 'if [[ -s answers.txt ]]; then exec ./bomb answers.txt; else exec ./bomb; fi'
        ;;

    gdb)
        ensure_container_running
        function_name="${2:-}"

        if [[ -n "$function_name" ]]; then
            require_function "$function_name"
        fi

        gdb_arguments=(
            gdb -q ./bomb
            -x ./gdb-bomb.gdb
        )

        if [[ -n "$function_name" ]]; then
            # 这里仅保存 Rosetta 解析出的运行时地址，不插断点。
            # 用户在首次输入处 Ctrl-C 后运行 bomb-arm。
            gdb_arguments+=( -ex "set \$bomb_target = (long)&${function_name}" )
            gdb_arguments+=( -ex "printf \"目标是 ${function_name}。先输入 continue；终端1出现欢迎文字后，在这里按 Ctrl-C，再输入 bomb-arm 和 continue。\\n\"" )
        fi

        docker exec -it \
            --workdir "$CONTAINER_BOMB_DIR" \
            "$CONTAINER_NAME" \
            "${gdb_arguments[@]}"
        ;;

    asm)
        ensure_container_running
        function_name="${2:-}"
        if [[ -z "$function_name" ]]; then
            echo "用法：./debug.sh asm 函数名" >&2
            exit 1
        fi
        require_function "$function_name"
        docker exec --workdir "$CONTAINER_BOMB_DIR" "$CONTAINER_NAME" \
            objdump -d -M intel --prefix-addresses \
                --disassemble="$function_name" ./bomb
        ;;

    addr)
        ensure_container_running
        function_name="${2:-}"
        if [[ -z "$function_name" ]]; then
            echo "用法：./debug.sh addr 函数名" >&2
            exit 1
        fi
        require_function "$function_name"
        function_range="$(symbol_range "$function_name")"
        echo "${function_name} 的起止地址："
        printf '%s\n' "$function_range"
        ;;

    functions)
        ensure_container_running
        search_word="${2:-}"
        docker exec --workdir "$CONTAINER_BOMB_DIR" "$CONTAINER_NAME" \
            sh -c 'nm -n --defined-only ./bomb | awk '\''$2 == "T" || $2 == "t" { print "0x" $1, $3 }'\'' | grep -i -- "$1" || true' sh "$search_word"
        ;;

    reset)
        require_docker
        if ! container_exists; then
            echo "错误：找不到容器 ${CONTAINER_NAME}。" >&2
            exit 1
        fi
        echo "正在重启 ${CONTAINER_NAME}，并结束其中残留的 Bomb/GDB 进程..."
        docker restart "$CONTAINER_NAME" >/dev/null
        echo "重启完成。项目文件不会被删除。"
        ;;

    status)
        require_docker
        if ! container_exists; then
            echo "容器：不存在 ($CONTAINER_NAME)"
            exit 1
        fi
        docker inspect --format '容器={{.Name}} 运行中={{.State.Running}} 架构={{.Platform}}' "$CONTAINER_NAME"
        docker exec "$CONTAINER_NAME" sh -lc 'ps -ef | grep -E "[b]omb|[g]db" || true' 2>/dev/null || true
        ;;

    help|-h|--help)
        usage
        ;;

    *)
        echo "错误：未知命令 $command_name" >&2
        usage >&2
        exit 1
        ;;
esac
