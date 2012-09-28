# I use this as a simple debugger for rarvm programs.

break rarvm.cpp:190

commands 1
    p Cmd->OpCode
    p/x R
    p/x Flags
    x/x Op1
    x/x Op2
end
