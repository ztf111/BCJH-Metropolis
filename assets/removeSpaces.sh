rm build_js/bcjh_*.data
cat data/data.min.json|awk '{printf "%s",$0}'|sed 's/ //g' > data/data; mv data/data data/data.min.json