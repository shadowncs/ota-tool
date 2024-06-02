package cmd

import (
	"fmt"
	"log"
	"os"
	"time"

	"github.com/spf13/pflag"

	"github.com/EmilyShepherd/ota-tool/internal/cmdutils"
)

type Apply struct {
	partitions []string
	input      string
	output     string
}

func (a *Apply) Usage() string {
	return "Applies the patch to existing partitions"
}

func (a *Apply) SetFlags(f *pflag.FlagSet) {
	f.StringVarP(&a.input, "input", "i", "", "The input Directory")
	f.StringVarP(&a.output, "output", "o", "", "The output directory")
	f.StringArrayVarP(&a.partitions, "partitions", "p", []string{}, "A list of partitions to apply. If not specified, all will be applied")
}

func (a *Apply) Execute(f *pflag.FlagSet) {
	update := cmdutils.LoadPayload(f.Arg(0))

	now := time.Now()

	if a.output == "" {
		a.output = fmt.Sprintf("extracted_%d%02d%02d_%02d%02d%02d", now.Year(), now.Month(), now.Day(), now.Hour(), now.Minute(), now.Second())
	}
	if _, err := os.Stat(a.output); os.IsNotExist(err) {
		if err := os.Mkdir(a.output, 0755); err != nil {
			log.Fatal("Failed to create target directory")
		}
	}
	update.SetConcurrency(4)

	if err := update.ExtractSelected(a.input, a.output, a.partitions); err != nil {
		log.Fatal(err)
	}
}
