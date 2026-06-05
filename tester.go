package main

import "os"
import "os/exec"
import "fmt"
import "path/filepath"
import "sync"

var exec_path string
var decoder_exec string
var samples_path string
var test_results []string

var sample_count int
var pass_count int

func main() {
	exec, err := os.Executable()
	if err != nil {
		panic(err)
	}
	
	exec_path = filepath.Dir(exec)
	decoder_exec = exec_path + "/decode"
	samples_path = exec_path + "/../samples"

	samples_dir, err := os.ReadDir(samples_path)
	if err != nil {
		panic(err)
	}
	
	sample_filenames := []string{}
	for _, file := range samples_dir {
		if !file.IsDir() && filepath.Ext(file.Name()) == "" {
			sample_filenames = append(sample_filenames, file.Name())
		}
	}

	sample_count = len(sample_filenames)

	test_results = make([]string, sample_count)

	wg := sync.WaitGroup{}

	for i, filename := range sample_filenames {
		wg.Add(1)
		go func() {
			tester_compare(filename, i)
			wg.Done()
		}()
	}
	
	wg.Wait()
	
	for _, res := range test_results {
		fmt.Println(res)
	}
	
	fmt.Printf("(%v/%v passed)\n", pass_count, sample_count)
}

func tester_compare(filename string, index int) {
	/* NOTE: Extremely error prone. It just works */

	var status string
	var matched bool
	
	sample_bin_filename    := fmt.Sprintf("%v/%v", samples_path, filename)
	generated_asm_filename := fmt.Sprintf("%v/tmp_%v.asm", exec_path, index)
	generated_bin_filename := fmt.Sprintf("%v/tmp_%v", exec_path, index)

	decode_cmd := exec.Command(decoder_exec, "-o", generated_asm_filename, sample_bin_filename)
	nasm_cmd   := exec.Command("nasm", "-o", generated_bin_filename, generated_asm_filename)
	cmp_cmd    := exec.Command("cmp", sample_bin_filename, generated_bin_filename)

	if err := decode_cmd.Run(); err != nil {
		if _, ok := err.(*exec.ExitError); ok {
			matched = false
			goto result
		} else {
			defer panic(err)
			goto cleanup
		}
	}
	
	if err := nasm_cmd.Run(); err != nil {
		if _, ok := err.(*exec.ExitError); ok {
			matched = false
			goto result
		} else {
			defer panic(err)
			goto cleanup
		}
	}
	
	if err := cmp_cmd.Run(); err != nil {
		if exiterr, ok := err.(*exec.ExitError); ok {
			if exiterr.ExitCode() == 1 {
				matched = false
			} else {
				fmt.Println("ERROR: Unexpected Exit Code from cmp command")
				defer panic(err)
				goto cleanup
			}
		} else {
			defer panic(err)
			goto cleanup
		}
	} else {
		matched = true
	}
	
result:	
	if matched {
		status = "1"
		pass_count += 1
	} else {
		status = "0"
	}

	test_results[index] = fmt.Sprintf("[%v] %v", status, filename)

cleanup:
	os.Remove(generated_asm_filename)
	os.Remove(generated_bin_filename)	
}


