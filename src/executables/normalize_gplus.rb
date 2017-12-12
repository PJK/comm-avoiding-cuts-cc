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

STDIN.each_line do |line|
	from, to = *line.scanf("%u %u\n")
	$edges += 1
	puts "#{translate(from)} #{translate(to)} 1"

	STDERR.puts $edges / 100_000 if $edges % 100_000 == 0
end

STDERR.puts <<-BANNER
Add the following at the beginning of the file:
# Comment
#{$vertices} #{$edges}
BANNER
