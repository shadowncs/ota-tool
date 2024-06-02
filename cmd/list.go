package cmd

import (
	"os"

	humanize "github.com/dustin/go-humanize"
	"github.com/olekukonko/tablewriter"
	"github.com/spf13/pflag"

	"github.com/EmilyShepherd/ota-tool/internal/cmdutils"
)

type List struct{}

func (l *List) Usage() string {
	return "Lists the partitions found in the payload"
}

func (l *List) SetFlags(f *pflag.FlagSet) {
	//
}

func (l *List) Execute(f *pflag.FlagSet) {
	update := cmdutils.LoadPayload(f.Arg(0))
	table := tablewriter.NewWriter(os.Stdout)
	table.SetHeader([]string{"Partition", "Old Size", "New Size"})
	for _, partition := range update.Partitions {
		table.Append([]string{
			partition.GetPartitionName(),
			humanize.Bytes(*partition.GetOldPartitionInfo().Size),
			humanize.Bytes(*partition.GetNewPartitionInfo().Size),
		})
	}
	table.SetBorder(false)
	table.Render()
}
