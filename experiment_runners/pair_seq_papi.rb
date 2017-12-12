#!/usr/bin/env ruby


ARGV.each do |input|
	all = IO.read(input).lines.reject { |_| _.include? '[unset]' }
	papi, measurement = all.partition { |_| _.start_with? 'PAPI' }

	res = ""
	papi.zip(measurement).each { |_| res << _[0]; res <<  _[1] }
	IO.write(input, res)
end
