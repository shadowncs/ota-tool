package cmd

import (
	"os"
	"strconv"

	"github.com/olekukonko/tablewriter"
	"github.com/spf13/pflag"

	"github.com/EmilyShepherd/ota-tool/internal/cmdutils"
)

type Show struct{}

func (s *Show) Usage() string {
	return "Applies a update to the given image(s)"
}

func (s *Show) SetFlags(f *pflag.FlagSet) {
	//
}

func (a *Show) Execute(f *pflag.FlagSet) {
	update := cmdutils.LoadPayload(f.Arg(0))
	partition := f.Arg(1)

	table := tablewriter.NewWriter(os.Stdout)
	table.SetHeader([]string{"Operation", "Start Block", "End Block"})

	for _, part := range update.Partitions {
		if partition == *part.PartitionName {
			for i, op := range part.Operations {
				for _, dst := range op.DstExtents {
					table.Append([]string{
						op.GetType().String() + " (" + strconv.Itoa(i) + ")",
						strconv.Itoa(int(dst.GetStartBlock())),
						strconv.Itoa(int(dst.GetStartBlock() + dst.GetNumBlocks())),
					})
				}
			}
			table.SetAutoMergeCellsByColumnIndex([]int{0})
			table.SetRowLine(true)
			table.Render()
			return
		}
	}
}
