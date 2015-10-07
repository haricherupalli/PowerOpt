proc echoPtCmd {cmd} {
    puts $cmd
}

proc setPtVar {var val} {
    global $var
    set $var $val
}

proc printPtVar {var} {
    global $var
    puts -nonewline "pt variable $var is "
    puts [expr $$var]
    return $$var
}

#### this is a test routine
proc testPrintList { var } {
#  foreach item $cellList {
##    if { [llength $item ] > 1 } {
##      testPrintList $item
##    } else 
#      puts $item
#  }
   puts $var
}


proc getPtServerHostname {} {
    return [exec hostname]
}

proc exitPtServer {} {
    exit
}

# perform swap cell and return the result of the operation
proc PtSwapCell {cell_instance cell_master} {
  #swap_cell $cell_instance proj/$cell_master
  set swap_status [swap_cell $cell_instance $cell_master]
  return $swap_status
}

proc PtReportTiming {cycle_time} {
    set report_default_significant_digits 6
    set timing_save_pin_arrival_and_slack true
    set timing_report_always_use_valid_start_end_points false
#    source /home/projects/SensOpt/templates/script/report_qor.tcl
#    report_qor
    echo "Running Cycle at cycle_time $cycle_time"
    report_timing -nosplit
}

proc PtResetPathThrough {cell} {
  set cmd "reset_path -through $cell"
  set status [eval $cmd]
  return status
}

proc PtResetPathTo {cell} {
  set cmd "reset_path -to $cell"
  set status [eval $cmd]
  return status
}

proc PtResetPathFrom {cell} {
  set cmd "reset_path -from $cell"
  set status [eval $cmd]
  return status
}

#PtResetRisePathforTerm {pin} 
proc PtRRPT {pin} {

  #echo "reset_path -rise_through $pin"
  reset_path -rise_through $pin
  ##echo "Done"
}

#PtResetFallPathForTerm
proc PtRFPT {pin} {

  #echo "reset_path -rise_through $pin"
  reset_path -rise_through $pin
  ##echo "Done"
}

#PtResetPathForTerm
proc PtRPT {pin} {

  #echo "reset_path -rise_through $pin"
  #echo "reset_path -fall_through $pin"
  
  reset_path -rise_through $pin
  reset_path -fall_through $pin
  ##echo "Done"
}

#PtSetFalsePathForTerm
proc PtSFPT { pin } {
  # Just through should be enough 

  #echo "set_false_path -rise_through $pin"
  #echo "set_false_path -fall_through $pin"

  set_false_path -rise_through $pin 
  set_false_path -fall_through $pin
  update_timing
  flush
  ##echo "Done"
}

proc PtGetMaxFallSlack { pin_name } {

  #echo "get_attribute \[get_pin $pin_name\] max_fall_slack"
  set max_fall_slack [get_attribute [get_pin $pin_name] max_fall_slack]
  #echo "Done $max_fall_slack"
  return $max_fall_slack
}


proc PtGetMaxRiseSlack { pin_name } {
  #echo "get_attribute \[get_pin $pin_name\] max_rise_slack"
  set max_rise_slack [get_attribute [get_pin $pin_name] max_rise_slack]
  #echo "Done $max_rise_slack"
  return $max_rise_slack
}

proc PtGetMaxFallArrival { pin_name } {

  #echo "get_attribute \[get_pin $pin_name\] max_fall_slack"
  set arrival  "ARRIVAL"
  set path [get_timing_path -pba exhaustive -fall_through $pin_name]  
  set size [sizeof_collection $path]
  if { $size == 0 } { return $arrival }
  set pts [get_attribute $path points]
  foreach_in_collection pt $pts {
    set pt_name [get_object_name [get_attribute $pt object ] ] 
    if { $pt_name == $pin_name } { 
        set arrival [get_attribute $pt arrival]
    }
  }
  return $arrival
  #set max_fall_arrival [get_attribute [get_pin $pin_name] max_fall_arrival]
  #echo "Done $max_fall_slack"
  #return $max_fall_arrival
}

proc PtGetMaxRiseArrival { pin_name } {
  #echo "get_attribute \[get_pin $pin_name\] max_rise_slack"
  set arrival  "ARRIVAL"
  set path [get_timing_path -pba exhaustive -rise_through $pin_name]  
  set size [sizeof_collection $path]
  if { $size == 0 } { return $arrival }
  set pts [get_attribute $path points]
  foreach_in_collection pt $pts {
    set pt_name [get_object_name [get_attribute $pt object ] ] 
    if { $pt_name == $pin_name } { 
        set arrival [get_attribute $pt arrival]
    }
  }
  return $arrival
#  set max_rise_arrival [get_attribute [get_pin $pin_name] max_rise_arrival]
#  #echo "Done $max_rise_slack"
#  return $max_rise_arrival
}


proc PtResetPathForCell {cell} {
  #echo "Reseting Paths for cell $cell"
  reset_path -rise_from $cell
  reset_path -rise_to $cell
  reset_path -rise_through $cell
  reset_path -fall_from $cell
  reset_path -fall_to $cell
  reset_path -fall_through $cell
}

proc PtResetPathForCellRise {cell} {
  #echo "Reseting Paths rise for cell $cell"
  reset_path -rise_from $cell
  reset_path -rise_to $cell
  reset_path -rise_through $cell
}

proc PtResetPathForCellFall {cell} {
  #echo "Reseting Paths fall for cell $cell"
  reset_path -fall_from $cell
  reset_path -fall_to $cell
  reset_path -fall_through $cell
}

proc PtReportQoR {} {
    source /home/projects/SensOpt/templates/script/report_qor.tcl
    report_qor > report.qor
}

proc PtSetBasicFalsePaths { } {

set_false_path -from dbg_uart_rxd
set_false_path -to   dbg_uart_txd

set_false_path -from nmi
set_false_path -from lfxt_clk
set_false_path -from reset_n

set_false_path -from cpu_en
set_false_path -from dbg_en

set disable_case_analysis false
set_case_analysis 0 [get_ports "dbg_en"]
set_case_analysis 0 [get_ports "scan_enable"]
set_case_analysis 0 [get_ports "scan_mode"]

}

proc PtSetFalsePathsForCells {} {
  #echo "Setting False paths for all cells"
  set_false_path -rise_through [ get_cells -hierarchical -filter "is_combinational or is_sequential"]
  set_false_path -rise_to      [ get_cells -hierarchical -filter "is_combinational or is_sequential"]
  set_false_path -rise_from    [ get_cells -hierarchical -filter "is_combinational or is_sequential"]
  set_false_path -fall_through [ get_cells -hierarchical -filter "is_combinational or is_sequential"]
  set_false_path -fall_to      [ get_cells -hierarchical -filter "is_combinational or is_sequential"]
  set_false_path -fall_from    [ get_cells -hierarchical -filter "is_combinational or is_sequential"]
#  set_false_path -through [get_cells -hierarchical U*]
#  set_false_path -to [get_cells -hierarchical U*]
#  set_false_path -from [get_cells -hierarchical U*]
#  set_false_path -through [get_cells -hierarchical *reg*]
#  set_false_path -to [get_cells -hierarchical *reg*]
#  set_false_path -from [get_cells -hierarchical *reg*]
}

proc PtSetFalsePathsForPins {} {
  # Just through should be enough 
#  echo "set_false_path -rise_through [get_pins -of_objects [get_cells -hierarchical -filter \"is_combinational or is_sequential\" ] ]"
#  echo "set_false_path -fall_through [get_pins -of_objects [get_cells -hierarchical -filter \"is_combinational or is_sequential\" ] ]"
   echo  " Setting  False Path through all pins " 

  set_false_path -rise_through [get_pins -of_objects [get_cells -hierarchical -filter "is_combinational or is_sequential"] ]
  set_false_path -fall_through [get_pins -of_objects [get_cells -hierarchical -filter "is_combinational or is_sequential"] ]
}

proc PtGetClkTreeCells {} {
    echo "Getting Clk Tree Cells"
    set clk_nwrk [get_clock_network_objects -type cell]
    set ret_val ""
    echo "Got cells"
    foreach obj $clk_nwrk {
        set ret_val [append $ret_val [get_attribute $obj full_name] ] 
    }
    echo "Done with the string. Now returning"
    return $ret_val
}


# The below procedure can get really slow 
proc PtResetPathsForCells {} { 
  echo "Reseting all paths"
  reset_path -through [get_cells -hierarchical]
  reset_path -from  [get_cells -hierarchical]
  reset_path -to  [get_cells -hierarchical]
  PtSetBasicFalsePaths
}

# perform timing analysis with the specified input clock cycle
# time and return the worst slack on the entire design
proc PtGetWorstSlack {clock_cycle_time} {
  #set half_cycle_time [expr {$clock_cycle_time/2.0}]

  # first change the clock constraint on the design (based on the input cycle time)
  #create_clock -name "CK" -add -period $clock_cycle_time -waveform {0 $half_cycle_time} [get_ports CK]
  #create_clock -name "CK" -add -period $clock_cycle_time [get_ports CK]

  #set_false_path -through U10059
  echo "Computing worst_slack"
  set paths [get_timing_path -delay max]
  set worst_slack 10000
  #set size [sizeof_collection $paths]
  foreach_in_collection path $paths {
    set slack [get_attribute $path slack]
    if {$slack < $worst_slack} {
      set worst_slack $slack
    }
  }
  echo "worst_slack is $worst_slack"
  return $worst_slack
}

proc PtGetWorstSlackPBA {clock_cycle_time} {
  #set half_cycle_time [expr {$clock_cycle_time/2.0}]

  # first change the clock constraint on the design (based on the input cycle time)
  #create_clock -name "CK" -add -period $clock_cycle_time -waveform {0 $half_cycle_time} [get_ports CK]
  #create_clock -name "CK" -add -period $clock_cycle_time [get_ports CK]

  #set_false_path -through U10059
  set paths [get_timing_path -delay max -pba_mode exhaustive]
  set worst_slack 10000
  #set size [sizeof_collection $paths]
  foreach_in_collection path $paths {
    set slack [get_attribute $path slack]
    if {$slack < $worst_slack} {
      set worst_slack $slack
    }
  }
  return $worst_slack
}

proc PtSuppressMessage {msg} {
  echo "Suppressing $msg"
  suppress_message $msg
}

# return worst path slack between flops
proc PtGetWorstSlackBetweenFlops {flop1 flop2} {
  set data_path [get_timing_path -from $flop1 -to $flop2 -delay max]
  set max_worst 10000
  foreach_in_collection each_path $data_path {
    set worst_pa th_slack [get_attribute $each_path slack]
    if {$worst_path_slack < $max_worst} {
      set worst_data_path_slack $worst_path_slack
    }
  }
  return $worst_data_path_slack
}


proc report_wire_delay {drvPinName  recPinName} {
    if { [catch [get_ports $drvPinName]] == 0 } {
        set drvPin [get_pin $drvPinName]
    } else {
        set drvPin [get_ports $drvPinName]
    }
    #set drvCell [get_cells -of $drvPin]
    if { [catch [get_ports $recPinName]] == 0 } {
        set recPin [get_pin $recPinName]
    } else {
        set recPin [get_ports $recPinName]
    }
    #set recCell [get_cells -of $recPin]

    # Find most critical timing-arc for cells and
    # find corresponding interconnect delay and slack
    set maxTimingPath [get_timing_path -nworst 1 -from $drvPin -to $recPin -delay max]

    set findDrv 0
    set findRec 0
    foreach_in_collection point [get_attri -quiet $maxTimingPath points] {
        set object [get_attri -quiet $point object]
        set objectName [get_object_name $object]
        if { $objectName == $drvPinName } {
            set drvArrival [get_attri $point arrival]
            # puts $drvArrival
            set findDrv 1
        } elseif { $objectName == $recPinName } {
            set recArrival [get_attri $point arrival]
            # puts $recArrival
            set findRec 1
        }
    }
    if { $findDrv != 1} {
        puts "Error: Cannot find arrival time of $drvPinName"
        puts "     : It can be because no such a pin"
    }
    if { $findRec != 1} {
        puts "Error: Cannot find arrival time of $recPinName"
        puts "     : It can be because no such a pin"
    }
    set wireDelay [expr $recArrival - $drvArrival]
    return $wireDelay
}

proc report_timing_arc { outPinName } {
    set outPin [get_pins $outPinName]

    # Find most critical timing-arc for cells and
    # find corresponding interconnect delay and slack
    set maxTimingPath [get_timing_path -nworst 1 -to $outPin -delay max]

    foreach_in_collection point [get_attri -quiet $maxTimingPath points] {
        set object [get_attri -quiet $point object]
        set objectName [get_object_name $object]

        if { $outPinName == $objectName } {
            # calc cell delay and slack
            set cell [get_cells -of $outPin]
            set cellName [get_attri $cell full_name]
            # Delay difference between previous timing point and current timing point
            set cellDelay [expr [get_attri $point arrival] - $predArrival]
            # Slack of source pin
            set cellSlack [get_attri $point slack]
            # Find polarity
            set cellRiseFall [get_attri $point rise_fall]
            # Arrival time for source pin
            set sourceArrival [get_attri [get_pins $objectName] max_${cellRiseFall}_arrival]
        } else {
            # Save predecessor delay
            set predName $objectName
            set predArrival [get_attri $point arrival]
        }
    }

    return $cellDelay
}

proc gate_delay { inPinName outPinName} {
    set inPin [get_pins $inPinName]

    # Find most critical timing-arc for cells and
    # find corresponding interconnect delay and slack
    set maxTimingPath [get_timing_path -nworst 1 -to $inPin -delay max]

    foreach_in_collection point [get_attri -quiet $maxTimingPath points] {

        set object [get_attri -quiet $point object]
        set objectName [get_object_name $object]

        if { $inPinName == $objectName } {
            # Save predecessor delay
            set predName $objectName
            set predArrival [get_attri $point arrival]
        } elseif { $outPinName == $objectName } {
            # calc cell delay and slack
            # Delay difference between previous timing point and current timing point
            set cellDelay [expr [get_attri $point arrival] - $predArrival]
        }
    }

    return $cellDelay
}

# return best path slack between flops
proc PtGetBestSlackBetweenFlops {flop1 flop2} {
  set data_path [get_timing_path -from $flop1 -to $flop2 -delay min]
  set min_worst -10000
  foreach_in_collection each_path $data_path {
    set best_path_slack [get_attribute $each_path slack]
    if {$best_path_slack > $min_worst} {
      set best_data_path_slack $best_path_slack
    }
  }
  return $best_data_path_slack
}

# return the (effective) load capacitance of a buffer
proc PtGetBufferLoadCap {buffer} {
  set buffer_output_pin_cap [get_attribute -class pin $buffer/Y effective_capacitance_max]
  return $buffer_output_pin_cap
}

proc PtGetFFDelay {pin} {
    set rise_arrival [get_attribute [get_pins $pin] max_rise_arrival]
    set fall_arrival [get_attribute [get_pins $pin] max_fall_arrival]
    puts "$pin max_rise_arrival: $rise_arrival\n"
    puts "$pin max_fall_arrival: $fall_arrival\n"

    set temp [lindex [list $rise_arrival $fall_arrival]]

    return $temp
}

# return the (maximum rise) input slew of a cell
proc PtGetCellInputSlew {pin} {
    set input_pin_rise_slew [get_attribute [get_pins $pin] actual_rise_transition_max]
    set input_pin_fall_slew [get_attribute [get_pins $pin] actual_fall_transition_max]
    puts "$pin rise slew: $input_pin_rise_slew\n"
    puts "$pin fall slew: $input_pin_fall_slew\n"

    set slew [lindex [list $input_pin_rise_slew $input_pin_fall_slew]]

    return $slew
}

proc PtGetCellLoadCap {pin} {
    set load_cap [get_attribute [get_pins $pin] ceff_params_max]
    puts "$pin load capacitance: $load_cap\n"

    return $load_cap
}


proc report_outcap { outPinName } {

    set outPin [get_pins $outPinName]
    # Find most critical timing-arc for cells and
    # find corresponding interconnect delay and slack
    set net [all_connected $outPin]
    set outCap [get_attri [get_net $net] total_capacitance_max]
    return $outCap
}

# return the (maximum rise) input slew of a buffer
proc PtGetBufferInputSlew {buffer} {
    set buffer_input_pin_slew [get_attribute -class pin $buffer/A actual_rise_transition_max]
    return $buffer_input_pin_slew
}

# return the master corresponding to a clock buffer
proc PtGetMasterOfInstance {buffer_cell_instance} {
  set buffer_cell_master [get_attribute -class cell $buffer_cell_instance ref_name]
  return $buffer_cell_master
}

# return the capactive shielding factor of interconnect from driver `cell_driver`  to load `cell_load`
proc PtGetCapacitiveShieldingFactor {driver_cell load_cell new_load_master} {
  #
  # with the existing cell masters of driver and load, a specific load cap seen
  # by the driver.
  #
  # we want to see how this load cap changes with change in input cap of the load
  # (i.e., how much load capacitance does the driver see now)
  #
  # we want to know the ratio!


  # compute output cap before swap
  set driver_output_cap_before_swap [get_attribute -class pin $driver_cell/Y effective_capacitance_max]

  # perform swap cell now

  # wait!!! store the actual master so that you can revert back to it after the swap
  set orig_load_cell_master [get_attribute -class cell $load_cell ref_name]

  set cap_swap_status [swap_cell $load_cell proj/$new_load_master]

  # perform update_timing
  #update_timing

  #compute output cap after swap
  set driver_output_cap_after_swap [get_attribute -class pin $driver_cell/Y effective_capacitance_max]

  # reverse the swap (i.e., restore the netlist to original state)
  set orig_swap_status [swap_cell $load_cell proj/$orig_load_cell_master]

  # compute the ratio
  set cap_shielding_factor [expr $driver_output_cap_after_swap - $driver_output_cap_before_swap]

  return $cap_shielding_factor
}

#################################################################################
#SHK

proc comparePath {list1 list2} {
	set common 0
	set commonPathList ""
	foreach l1 $list1 {
		foreach l2 $list2 {
			if { $l1 == $l2 } {
				return $l1
			}
		}
	}
}
##########################################################################
# Find driver of given input or output pin of a buffer or inverter
# Argument
#    pin name
# Return value
#    driver pin or port
##########################################################################
proc findDriver { pinName } {
	if { [get_attri [get_pins $pinName] direction ] == "in" } {
		set net [all_connected $pinName]
		set pins [all_connected $net -leaf ]
		foreach_in_collection pin $pins {
			set cpinName  [get_attri $pin full_name]
			set cpin ""
			set cpin [get_pins $cpinName]
			if {$cpin == "" } {
				# port
				set cpin [get_port $cpinName]
				set direction [get_attri $cpin direction]
				if {$direction == "in" } {
					return $cpinName
				}
			} else {
				set direction [get_attri $cpin direction]
				if {$direction == "out" } {
					return $cpinName
				}
			}
		}
	} else {
		set cell [get_cell -of [get_pin $pinName]]
		set inPin [get_pin -of $cell -filter "direction==in"]
		set net [all_connected $inPin]
		set pins [all_connected $net -leaf ]
		foreach_in_collection pin $pins {
			set cpinName  [get_attri $pin full_name]
			set cpin ""
			set cpin [get_pins $cpinName]
			if {$cpin == "" } {
				set cpin [get_port $cpinName]
				set direction [get_attri $cpin direction]
				if {$direction == "in" } {
					return $cpinName
				}
			} else {
				set direction [get_attri $cpin direction]
				if {$direction == "out" } {
					return $cpinName
				}
			}
		}
	}
}
##########################################################################
# Calculate the number of common clock cells
# Argument
#    flip-flop1 flop-flop2
# Return value
#    the number of common clock cells
##########################################################################
proc PtFindLCA {ff1Name ff2Name} {
	set ff1 [get_cell $ff1Name]
	set ff2 [get_cell $ff2Name]
	set source [get_attri [get_clock] full_name]

	set startPin [get_pins -of $ff1 -filter "is_clock_pin==true" ]
	set startPath ""

	set currentPin $startPin
	set currentPinName [get_attri $currentPin full_name]
	while { $currentPinName != $source } {
		set currentPinName [findDriver $currentPinName]
		set cellName [get_attri [get_cell -of [get_pin $currentPinName]] full_name]
		set startPath "$startPath $cellName"
	}

	set endPin [get_pins -of $ff2 -filter "is_clock_pin==true" ]
	set endPath ""

	set currentPin $endPin
	set currentPinName [get_attri $currentPin full_name]
	while { $currentPinName != $source } {
		set currentPinName [findDriver $currentPinName]
		set cellName [get_attri [get_cell -of [get_pin $currentPinName]] full_name]
		set endPath "$endPath $cellName"
	}
	#puts $startPath
	#puts $endPath

	return [comparePath $startPath $endPath]
}

# Return Cell list in the WNS path


proc PtListInstance { clkName } {
    set cellList ""
    #set path [get_timing_path -max_path 1 -group $clkName ]
    set path [get_timing_path -max_path 1 -group $clkName -from [all_registers -clock_pins] -to [all_registers -data_pins]]
    set first 1
    foreach_in_collection point [get_attri $path points] {
        set object [get_attri -quiet $point object]
        set objectName [get_object_name $object]
        set pin [get_pins $objectName]
        if { [sizeof_collection $pin] == 0 && $first == 1 } {
            set cellList "$cellList $objectName"
        } elseif { [sizeof_collection $pin] == 0 } {
            set cellEnd "$objectName"
        } else {
            set pinDirection [get_attri $pin direction]
            if {$pinDirection == "out" } {
                set pinName [get_attri $pin full_name]
                #puts $pinName
                set cellList "$cellList $pinName"
            }
            if {$pinDirection == "in" } {
                set pinName [get_attri $pin full_name]
                set cellEnd "$pinName"
            }
        }
        set first 0
    }
    set cellList "$cellList $cellEnd"
    puts "Cell List : $cellList \n"
    return $cellList
}

# Get Slack value of path

#proc PtPathSlack { path_string } {
#  set cmd "get_timing_path  "
#  foreach word $path_string {
#      set cmd "$cmd $word"
#  }
#  set path [eval $cmd]
#  set path_slack [get_attribute $path slack]
#  return $path_slack
#}
proc PtPathSlack { path_string } {
#    set oldPathString ""
#    set newPathString ""
#
#    foreach word $path_string {
#        if {[regexp -- "-through" $word]} {
#            continue
#        }
#        if {[regexp -- "-from" $word]} {
#            continue
#        }
#        if {[regexp -- "-to" $word]} {
#            continue
#        }
#        set oldPathString "$oldPathString $word"
#    }
#    set wordCount [llength $oldPathString]
#    for {set i 0 } { $i < [expr $wordCount-1] } {incr i} {
#        set word [lindex $oldPathString $i]
#        set pin ""
#        set pinIsPort 0
#        set pin [get_pins  $word]
#        if { $pin == "" } {
#            set pin [get_port $word]
#            set pinIsPort 1
#        }
#        if { $i == 0} {
#            set newPathString "\-from $word"
#            set nextWord [lindex $oldPathString 1]
#            set nextCell [get_cells -of [get_pin $nextWord]]
#            set nextCellName [get_attri $nextCell full_name]
#
#            if { $pinIsPort == 0 } {
#                set outPinList [get_pins -of [get_cell -of $pin] -filter "direction== out"]
#            } else {
#                set outPinList $pin
#            }
#   foreach_in_collection outPin $outPinList {
#                foreach_in_collection npin [all_connected [all_connected $outPin ] -leaf] {
#                    set nextPinName [get_attri $npin full_name]
#                    set name [get_attri [get_cell -of [get_pin $npin]] full_name]
#                    if { $name == $nextCellName } {
#                        set newPathString "$newPathString \-through $nextPinName"
#                    }
#                }
#            }
#        } else {
#            set newPathString "$newPathString \-through $word"
#        }
#        if {[get_attri $pin direction] == "out" } {
#            set nextWord [lindex $oldPathString [expr $i+1]]
#            set nextCell [get_cells -of [get_pin $nextWord]]
#            set nextCellName [get_attri $nextCell full_name]
#            foreach_in_collection npin [all_connected [all_connected $pin ] -leaf] {
#                set nextPinName [get_attri $npin full_name]
#                set name [get_attri [get_cell -of [get_pin $npin]] full_name]
#                if { $name == $nextCellName } {
#                    set newPathString "$newPathString \-through $nextPinName"
#                }
#            }
#        }
#    }
#    set newPathString "$newPathString -to [lindex $oldPathString [expr $wordCount -1]]"
#    set newPathString $path_string
###    set cmd "get_timing_path $newPathString"
    set cmd "get_timing_path $path_string"
    set path [eval $cmd]
    set path_slack [get_attri $path slack]
    return $path_slack
}


proc PtFFSlack { cell_name } {
    set pin [get_pins -of $cell_name -filter "is_data_pin == true"]
    set max_worst 10000
    foreach_in_collection each_pin $pin {
        set rise_slack [get_attribute $each_pin max_rise_slack]
        set fall_slack [get_attribute $each_pin max_fall_slack]
        if {$rise_slack < $fall_slack} {
            set worst_slack $rise_slack
        } else {
            set worst_slack $fall_slack
        }
        if {$worst_slack < $max_worst} {
            set max_worst $worst_slack
        }
    }
    return $max_worst
}

# return number of timing paths betweeen two F/Fs
proc PtGetSizePathFF {flop1 flop2} {
    set collection1 [get_cell [lindex $flop1 0]]
    for { set i 1 } { $i < [llength $flop1] } {incr i} {
        set collection1 [add_to_collection $collection1 [get_cell [lindex $flop1 $i]]]
    }
    set collection2 [get_cell [lindex $flop2 0]]
    for { set j 1 } { $j < [llength $flop2] } {incr j} {
        set collection2 [add_to_collection $collection2 [get_cell [lindex $flop2 $j]]]
    }

    set path [get_timing_path -from $collection1 -to $collection2]
    set sizePath [sizeof_collection $path]
    return $sizePath
}

proc PtSetCell {cell cell_name} {
    set $cell [get_cell $cell_name]
}

proc PtAddtoCol {cell cell_name} {
    add_to_collection $cell [get_cell $cell_name]
}

proc PtCellToggleRate { cell_name } {
    set pin [get_pins -of $cell_name]
    set max_trate 0
    foreach_in_collection each_pin $pin {
        set pinDirection [get_attri $each_pin direction]
        if {$pinDirection == "out" } {
            set trate [get_attribute $each_pin toggle_rate]
            if {$trate > $max_trate} {
                set max_trate $trate
            }
        }
    }
    return $max_trate
}

proc PtCellPower { cell_name } {
    set cell [get_cell $cell_name]
    set power [get_attri $cell total_power]

    return $power
}

proc PtCellLeak { cell_name } {
    set cell [get_cell $cell_name]
    set power [get_attri $cell leakage_power]

    return $power
}

proc PtTotalPower {} {
    set power [get_attri [current_design] total_power]
    return $power
}

proc PtLeakPower {} {
    set power [get_attri [current_design] leakage_power]
    return $power
}

proc PtArea {} {
    set AREA [get_attri [current_design] area]
    return $AREA
}

proc PtCellSlack { cell_name } {
    set pin [get_pins -of $cell_name]
    set max_worst 10000
    foreach_in_collection each_pin $pin {
        set pinDirection [get_attri $each_pin direction]
        if {$pinDirection == "out" } {
            set rise_slack [get_attribute $each_pin max_rise_slack]
            set fall_slack [get_attribute $each_pin max_fall_slack]
            if {$rise_slack < $fall_slack} {
                set worst_slack $rise_slack
            } else {
                set worst_slack $fall_slack
            }
            if {$worst_slack < $max_worst} {
                set max_worst $worst_slack
            }
        }
    }
    return $max_worst
}

proc PtCellDelay { cell_name } {
    set pin [get_pins -of $cell_name]
    set max_worst 0
    set out_arrival 0
    set max_delay 0
    set cell_delay 0
    foreach_in_collection each_pin $pin {
        set pinDirection [get_attri $each_pin direction]
        if {$pinDirection == "out" } {
            set rise_arrival [get_attribute $each_pin max_rise_arrival]
            set fall_arrival [get_attribute $each_pin max_fall_arrival]
            if {$rise_arrival > $fall_arrival} {
                set worst_arrival $rise_arrival
            } else {
                set worst_arrival $fall_arrival
            }
            if {$worst_arrival > $out_arrival} {
                set out_arrival $worst_arrival
            }
        }
    }
    #puts "out $out_arrival"
    foreach_in_collection each_pin $pin {
        set pinDirection [get_attri $each_pin direction]
        if {$pinDirection == "in" } {
            set rise_arrival [get_attribute $each_pin max_rise_arrival]
            if {$rise_arrival == ""} {
                continue
            }
            set fall_arrival [get_attribute $each_pin max_fall_arrival]
            if {$fall_arrival == ""} {
                continue
            }
            if {$rise_arrival > $fall_arrival} {
                set worst_arrival $rise_arrival
            } else {
                set worst_arrival $fall_arrival
            }
            set max_delay [expr $out_arrival - $worst_arrival]
            if {$max_delay > $cell_delay} {
                set cell_delay $max_delay
            }
            #puts "in $worst_arrival"
            #puts "delay $max_delay"
        }
    }
    return $cell_delay
}

proc PtWorstSlack { clk_name } {
    echo "Reporting Worst Slack and clock name is $clk_name"
    set path [get_timing_path -nworst 1 -group $clk_name]
    set path_slack [get_attribute $path slack]
    return $path_slack
}

proc PtWorstSlackMin { clk_name } {
    set path [get_timing_path -delay min -nworst 1 -group $clk_name]
    set path_slack [get_attribute $path slack]
    return $path_slack
}

proc PtCheckMaxTran { cellName } {
    set pins [get_pins -of $cellName]
    foreach_in_collection each_pin $pins {
        set const [get_attribute $each_pin max_transition]
        set tran_rise [get_attribute $each_pin actual_rise_transition_max]
        set tran_fall [get_attribute $each_pin actual_fall_transition_max]
        if {$tran_rise < $tran_fall} {
            set tran_max $tran_fall
        } else {
            set tran_max $tran_rise
        }
        if {$tran_max > $const} {
            return 0
        }
    }
    return 1
}

proc PtCheckPT {} {
    return 1
}

# get minimum slack from the cell (at in/output port)
proc PtMinSlack { CellName } {
    set max_worst 10000
    set pins [get_pins -of $CellName]
    foreach_in_collection each_pin $pins {
      set nets [all_connected $each_pin]
      foreach_in_collection each_net $nets {
        set alt_pins [all_connected $each_net -leaf]
        foreach_in_collection each_alt_pin $alt_pins {
          set rise_slack [get_attribute $each_alt_pin max_rise_slack]
          set fall_slack [get_attribute $each_alt_pin max_fall_slack]
          if {$rise_slack < $fall_slack} {
            set worst_slack $rise_slack
          } else {
            set worst_slack $fall_slack
          }
          if {$worst_slack < $max_worst} {
            set max_worst $worst_slack
          }
        }
      }
    }
    return $max_worst
}

proc PtGetLibCell { CellName } {
    set Cell [get_cell $CellName]
    set LibCell [get_lib_cells -of_objects $Cell]
    set LibCellName [get_attri $LibCell base_name]
    return $LibCellName
}

proc PtGetOutPin { CellName } {
    set pin [get_pins -of $CellName]
    foreach_in_collection each_pin $pin {
        set pinDirection [get_attri $each_pin direction]
        set OutPinName NULL
        if {$pinDirection == "out" } {
            set OutPinName [get_attribute $each_pin full_name]
        }
    }
    return $OutPinName
}

proc PtGetCellFromNet { NetName } {
    set net [get_net $NetName]
    set pins [all_connected $net -leaf]
    set CellName NULL_CELL
    set once true
    foreach_in_collection pin $pins {
        if { [get_attri $pin direction] == "out" && [get_attri $pin is_port] == false } {
           if { $once == true } {
             set CellName [get_attri [get_cell -of $pin] full_name]
             set once false
           } else {
             set CellName MULTI_DRIVEN_NET
           }
        }
    }
    #echo "net name is $NetName and its cell is $CellName"
    return $CellName
}

proc PtGetTermsFromNet { NetName } {
    #echo "net name is $NetName"
    set term_list ""
    set net [get_net $NetName]
    set pins [all_connected $net -leaf]
    foreach_in_collection pin $pins {
        set term_list [concat $term_list [get_attribute $pin full_name]]
    }
    #echo "term_list is $term_list"
    return $term_list
}

proc PtGetFanoutCells { CellName } {
    set return_list ""
    set pins [get_pin -of $CellName]
    foreach_in_collection pin $pins {
      if { [get_attri $pin direction] == "out" && [get_attri $pin is_port] == false} {
         set net [all_connected $pin -leaf]
         if { [sizeof_collection $net] > 1 } {
             return "ERROR_IN_PT"
          } else {
             set net_pins [all_connected $net -leaf]
              foreach_in_collection net_pin $net_pins {
                set cell [get_cell -of $net_pin]
                set cell_name [get_attri $cell full_name]
                if { $cell_name != $CellName } {
                  set return_list [concat "$return_list$cell_name,"]
                }
              }
          }
      }  
    }
   return $return_list
}

proc PtGetInst { PinName } {
  set Cell [get_cells -of $PinName]
  set CellName [get_attri $Cell full_name]
  return $CellName
}

proc PtRemoveFFDelay { CellName } {
  set cell [get_cell $CellName]
  set clockPin [get_pins -of $cell -filter "is_clock_pin==true"]
  set outPins [get_pins -of $cell -filter "direction==out"]
  foreach_in_collection out $outPins  {
    set_annotated_delay -cell -from $clockPin -to $out 0
  }
}

#proc PtSizeCell {PinName cell_master} {
#  #size_cell $cell_instance proj/$cell_master
#  set Cell [get_cells -of $PinName]
#  set size_status [size_cell $Cell $cell_master]
#  return $size_status
#}

proc PtSizeCell {Cell cell_master} {
  set size_status [size_cell $Cell $cell_master]
  return $size_status
}

proc PtSetFalsePathThrough { cell } {
  #echo "setting false path through the cell $cell"
  set cmd "set_false_path -through $cell"
  #echo $cmd
  set status [eval $cmd]
  return $status
}

proc PtSetFalsePathFrom { cell } {
  #echo "setting false path from the cell $cell"
  set cmd "set_false_path -from $cell"
  #echo $cmd
  set status [eval $cmd]
  return $status
}

proc PtSetFalsePathTo { cell } {
  #echo "setting false path to the cell $cell"
  set cmd "set_false_path -to $cell"
  #echo $cmd
  set status [eval $cmd]
  #echo $status
  return $status
}

#proc PtSetFalsePathsThrough { cell_list } {
#  set cmd "set false_path -through"
#  foreach word $cell_list {
#    set full_cmd "$cmd $word"
#    set status [eval $full_cmd]
#    if {$status = 0} {
#      echo "SetFalsePathsThrough Failed"
#      return $status
#    }
#  }
#  return $status
#}

proc PtSetFalsePath { path_string } {
  set cmd "set_false_path "
  foreach word $path_string {
      set cmd "$cmd $word"
  }
  set status [eval $cmd]
  return $status
}

proc PtUpdatePower { } {
  set cmd "update_power"
  set status [eval $cmd]
  return $status
}

proc PtUpdateTiming { } {
  set cmd "update_timing"
  set status [eval $cmd]
  return $status
}

proc PtResetPath { path_string } {
  set cmd "reset_path "
  foreach word $path_string {
      set cmd "$cmd $word"
  }
  set status [eval $cmd]
  return $status
}

proc PtWriteChange { FileName } {
  set cmd "write_changes -format ptsh -out $FileName"
  set status [eval $cmd]
  catch {exec touch $FileName}
  catch {exec echo "current_instance" >> $FileName}
  return $status
}

proc PtMakeResizeTcl { target1 target2  } {
  set inFp  [open change.tcl]
  set outFp [open $target2.tcl w]

  puts $outFp "current_instance"
  while { [gets $inFp line] >=0 } {
    if { [regexp {^size_cell} $line] } {
        regsub -all "{|}" $line "" temp
        regsub -all "$target1\/" $temp "" temp
        set inst [lindex $temp 1]
        set cell [lindex $temp 2]
        puts $outFp "size_cell \{$inst\} \{$cell\}"
    }
  }
  puts $outFp "current_instance"
  close $inFp
  close $outFp
}

proc PtMakeEcoTcl { file target } {
  set inFp  [open $file]
  set outFp [open $target w]
  while { [gets $inFp line] >=0 } {
    if { [regexp {current_instance} $line] && [llength $line] == 1 } {
        set dir ""
    } elseif { [regexp {current_instance} $line] && [llength $line] == 2 } {
        set dir [lindex $line 1]
        set dir "$dir/"
    } elseif { [regexp {^size_cell} $line] } {
        regsub -all {(\{[^\ ]+\/)} $line "" temp
        regsub -all "{|}" $temp "" temp
        set inst [lindex $temp 1]
        set cell [lindex $temp 2]
     puts $outFp "ecoChangeCell \-inst $dir$inst \-cell $cell"
    }
  }
  close $inFp
  close $outFp
}

proc PtMakeFinalEcoTcl { target } {
  set inFp  [open eco_$target\.tcl]
  set outFp [open eco.tcl w]

  while { [gets $inFp line] >=0 } {
     puts $outFp $line
  }
  close $inFp
  close $outFp
}

proc PtMakeLeakList { inFile outFile } {
  set inFp  [open $inFile]
  set outFp [open $outFile w]
  set count 9999
  set flag 1
  set foot "footprint"
  while { [gets $inFp line] >=0 } {
    set LeftBrace  [regsub -all {\{} $line "" temp]
    set RightBrace [regsub -all {\}} $line "" temp]
    if { [regexp {cell \(} $line] || [regexp {cell\(} $line] && ![regexp "test_cell" $line] } {
      regsub -all {\(|\)|\{|\}} $line "" temp
      set cell [lindex $temp 1]
      set count 0
      set flag 1
    } elseif { [regexp {cell_leakage_power } $line] && ![regexp "default_cell_leakage_power " $line] } {
      regsub -all ";" $line "" temp
      set leakage [lindex $temp 2]
      set flag 0
    } elseif { [regexp {cell_footprint } $line] } {
      regsub -all ";|\"" $line "" temp
      set foot [lindex $temp 2]
      set flag 0
    }
    set count [expr $count + $LeftBrace - $RightBrace]
    if { $count == 0 && $flag == 0 } {
      set count 9999
      puts $outFp "$cell\t$leakage\t$foot"
    }
  }
  close $inFp
  close $outFp
}

proc PtGetLibNames { } {
    set allLibName [get_attri [get_lib] full_name]
    return $allLibName
}

proc PtMakeCellList { libName outFile } {

  set allLibCells [get_lib_cell $libName/*]
  foreach_in_collection libCell $allLibCells {
    set functionId [get_attri $libCell function_id]
    if { [info exist cellList($functionId) ] } {
      set cellList($functionId) "$cellList($functionId) [get_attri $libCell base_name]"
    } else {
      set cellList($functionId) "[get_attri $libCell base_name]"
    }
  }

  set defaultSlew 0.1
  set defaultCap 0.02
  set outFp [open $outFile w]

  foreach id [array names cellList] {
    foreach cellName $cellList($id) {
      if { [get_attri [get_lib_cell $libName/$cellName] is_combinational] } {
        set libInPins [get_attri [get_lib_pin -of [get_lib_cell $libName/$cellName] -filter "direction==in"] base_name]
      } else {
        set libInPins [get_attri [get_lib_pin -of [get_lib_cell $libName/$cellName] -filter "clock==true && is_clear_pin==false && is_async_pin==false"] base_name]
      }
      set libOutPins [get_attri [get_lib_pin -of [get_lib_cell $libName/$cellName] -filter "direction==out"] base_name]
      report_driver_model -lib_cell $libName/$cellName -from_pin [lindex $libInPins 0] -to_pin [lindex $libOutPins 0] -capacitance $defaultCap -rise_slew $defaultSlew -fall_slew $defaultSlew  > temp.sensOpt
      set delayFile [open temp.sensOpt]
      set sumDelay 0
      while {[gets $delayFile line] >=0 } {
        if { [lindex $line 0] == "delay" && [lindex $line 1] == "=" } {
          set cellDelay [lindex $line 2]
          if { $cellDelay == "n/a" } { set cellDelay 0 }
          set sumDelay [expr $sumDelay + $cellDelay ]
        }
      }
      close $delayFile
      puts $outFp "$id\t$cellName\t$sumDelay"
    }
  }
  close $outFp
}


proc PtWriteVerilog { target } {
  set cmd "write_verilog -o $target"
  set status [eval $cmd]
  return $status

}

proc PtReadTcl { FileName } {
  set cmd "source $FileName"
  set status [eval $cmd]
  return $status
}

proc PtReportSlack { FileName } {
  set cmd "report_timing -group rclk -slack_lesser_than 0.20 -nosplit -max_paths 200000 > $FileName"
  set status [eval $cmd]
  return $status
}

proc PtWriteSDF { SDF } {
  write_sdf  $SDF
}

proc PtCommand { Command } {
  $Command
}

proc PtSaveSession { FileName } {
  save_session $FileName
}

proc PtRestoreSession { FileName } {
  remove_design -all
  restore_session $FileName
}

proc PtLinkLib { VOLT } {
  global link_path
  set link_path [list * tcbn65_v$VOLT\.db tcbn65_seq_v$VOLT\.db]
}

proc PtExit {} {
  set cmd "exit"
  set status [eval $cmd]
  return $status
}

proc PtGetFFName { } {
    set ff_list [all_registers]
    foreach_in_collection ff $ff_list {
        puts [get_attri $ff full_name]
    }
}

