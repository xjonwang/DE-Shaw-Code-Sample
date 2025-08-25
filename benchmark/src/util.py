import os
import sys

def fork_and_exec(program: str, *args) -> int:
    program = os.path.expanduser(program)
    pid: int = os.fork()
    if pid == 0:
        try:
            os.execl(program, program, *args)
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            sys.exit(1)
    else:
        return pid