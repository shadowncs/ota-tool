package cmdutils

import (
	"fmt"
	"log"
	"os"
	"strings"

	"github.com/EmilyShepherd/ota-tool/pkg/payload"
)

func LoadPayload(filename string) *payload.Payload {
	if _, err := os.Stat(filename); os.IsNotExist(err) {
		log.Fatalf("File does not exist: %s\n", filename)
		return nil
	}

	var update *payload.Payload
	var err error

	if strings.HasSuffix(filename, ".zip") {
		update, err = payload.NewPayloadFromZipFile(filename)
	} else {
		f, _ := os.Open(filename)
		update = payload.NewPayload(f)
	}

	if err != nil {
		fmt.Println(err)
		return nil
	}

	update.Init()

	return update
}
