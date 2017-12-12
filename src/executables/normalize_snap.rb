#!/usr/bin/env ruby

require 'scanf'

$mapping = Hash.new
$vertices, $edges = 0, 0

def translate(vertex)
	if $mapping.include? vertex
		$mapping[vertex]
	else
		$mapping[vertex] = $vertices
		$vertices += 1
		$vertices - 1
	end
end

# Remove duplicates, count edges, relabel all vertices to be in [0, N - 1]

STDIN.each_line do |line|
	next if line.start_with? "#"
	from, to = *line.scanf("%u\t%u\n")

	if translate(from) != translate(to)
		$edges += 1
		puts "#{translate(from)} #{translate(to)} 1"
	end

	STDERR.puts $edges / 100_000 if $edges % 100_000 == 0
end

STDERR.puts <<-BANNER
Add the following at the beginning of the file:
# Comment
#{$vertices} #{$edges}
BANNER
