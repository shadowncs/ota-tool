package command

import (
	"fmt"

	"github.com/spf13/pflag"
)

type Command interface {
	Usage() string
	Execute(*pflag.FlagSet)
	SetFlags(*pflag.FlagSet)
}

type Group struct {
	commands map[string]Command
}

func (c *Group) Register(name string, cmd Command) {
	if c.commands == nil {
		c.commands = make(map[string]Command)
	}
	c.commands[name] = cmd
}

func (g *Group) Usage() string {
	output := "Commands:\n"
	for name, cmd := range g.commands {
		output += fmt.Sprintf("  %s\t%s\n", name, cmd.Usage())
	}

	return output
}

func (g *Group) Execute(args []string) {
	for name, cmd := range g.commands {
		if name == args[0] {
			flag := pflag.NewFlagSet(name, pflag.ContinueOnError)
			cmd.SetFlags(flag)
			flag.Parse(args[1:])

			cmd.Execute(flag)
			return
		}
	}

	fmt.Println(g.Usage())
}
