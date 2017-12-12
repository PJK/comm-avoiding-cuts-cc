#!/usr/bin/env ruby

all = STDIN.read
all = all.lines.reject { |_| _.include?('slurm') || _.include?('srun') || _.include?('[unset]') }.join

segments = all.split(/\/scratch.+$/).map(&:strip)
headers = all.scan(/\/scratch.+$/).map(&:strip)

headers.zip(segments).each do |record|
	main = record.first.gsub("stoer-wagner,", "0,stoer-wagner,")
	papis = record[1].lines.map(&:chomp)
	papis.map! { |_| _.sub('PAPI,', '').split(",").map(&:to_f) }

	if papis.empty?
		puts "#{main},#{',' * 7}"
	else
		puts "#{main},#{papis.transpose.map(&:max).join(',')}"
	end
end

