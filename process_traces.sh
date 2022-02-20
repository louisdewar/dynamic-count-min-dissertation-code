
gunzip -f data.pcap.gz
tcpdump -q -n -t -r data.pcap | grep IP | sed 's/\./ /g' | awk '{print $2" "$3" "$4" "$5"\t"$8" "$9" "$10" "$11}' | grep -v "[a-z:]" > trace 2> /dev/null
rm data.pcap #save space