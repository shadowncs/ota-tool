package main

import (
	"os"
	"runtime"

	"github.com/EmilyShepherd/ota-tool/cmd"
	"github.com/EmilyShepherd/ota-tool/pkg/command"
)

func main() {
	runtime.GOMAXPROCS(runtime.NumCPU())

	root := command.Group{}
	root.Register("list", &cmd.List{})
	root.Register("apply", &cmd.Apply{})
	root.Register("show", &cmd.Show{})
	root.Execute(os.Args[1:])
}
