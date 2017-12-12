source("R/common.R")

raw32_data <- read.csv("data/e1/32k.csv")
ws32_data <- process_df(raw32_data[grepl("ws_", raw32_data$input), ], 0.05)

raw48_data <- read.csv("data/e1/48k.csv")
ws48_data <- process_df(raw48_data[grepl("ws_", raw48_data$input), ], 0.05)

raw64_data <- read.csv("data/e1/64k.csv")
ws64_data <- process_df(raw64_data[grepl("ws_", raw64_data$input), ], 0.05)

raw96_data <- read.csv("data/e1/96k.csv")
ws96_data <- process_df(raw96_data[grepl("ws_", raw96_data$input), ], 0.05)

options(scipen=5)
lcex <- 0.7
pcex <- 1.2
acex <- 0.7


baseline <- read.csv("data/e1/baselines.csv")
baseline32 <- process_df(baseline[grepl("ws_32", baseline$input), ], 0.05)
baseline48 <- process_df(baseline[grepl("ws_48", baseline$input), ], 0.05)
baseline64 <- process_df(baseline[grepl("ws_64", baseline$input), ], 0.05)
baseline96 <- process_df(baseline[grepl("ws_96", baseline$input), ], 0.05)

print(c(baseline32$median[1], baseline48$median[1], baseline64$median[1], baseline96$median[1]))

plotClass <- function(d1, d2, d3, d4,  limits = c(0.3, 4500)) {
  par(mar=c(3,3,1, 0.6))
  plot(d1$cores, d1$median, cex = pcex, pch = 18, log="y", 
	   ylim = limits, axes = F, xlab='', ylab = '', xlim = c(-50, 1440), type = "n")
  grid()  
  
  
  segments(loffset, baseline96$median[1], 1440, baseline96$median[1], col = "azure4")
  segments(loffset, baseline64$median[1], 1440, baseline64$median[1], col = "azure4")
  segments(loffset, baseline48$median[1], 1440, baseline48$median[1], col = "azure4")
  segments(loffset, baseline32$median[1], 1440, baseline32$median[1], col = "azure4")
  
  
  points(d1$cores, d1$median, cex = pcex, pch = 18)
  lines(tail(d1$cores, -1), tail(d1$median, -1), lty = 2)
  
  title(xlab = "Number of processors", ylab = "Running time [s]", cex.lab = lcex, line = 1.3)
  
  points(d2$cores, d2$median, cex = pcex, pch = 17, col = "darkred")
  lines(tail(d2$cores, -1), tail(d2$median, -1), col = "darkred", lty = 2)
  
  points(d3$cores, d3$median, cex = pcex, pch = 8, col = "darkgreen")
  lines(tail(d3$cores, -1), tail(d3$median, -1), col = "darkgreen", lty = 2)
  
  points(d4$cores, d4$median, cex = pcex, pch = 15, col = "darkblue")
  lines(tail(d4$cores, -1), tail(d4$median, -1), col = "darkblue", lty = 2)
  
  axis(side=1, at = d1$cores, lab = d1$cores, cex.axis = acex, tck = -0.03, mgp=c(3, .3, 0)); box()
  axis(side=2, cex.axis =acex, tck = -0.03, mgp=c(3, .5, 0))
  
  box()
  
  legend('topright', legend = c("n = 96'000", "n = 64'000", "n = 48'000", "n = 32'000"), 
		  inset = 0.01, pch = c(15, 8, 17, 18),
		 bty = "y",
		 col = c("darkblue", "darkgreen", "darkred", "black"), cex = lcex, bg = "white")
  
}
plotClass(ws32_data, ws48_data, ws64_data, ws96_data)

pdf("../paper/img/sparse_scaling_1.pdf", width = 4.7, height = 3)
plotClass(ws32_data, ws48_data, ws64_data, ws96_data)
loffset <- -70
#segments(loffset, baseline96$median[1], 0, baseline96$median[1], col = "azure4")
#segments(loffset, baseline64$median[1], 0, baseline64$median[1], col = "azure4")
#segments(loffset, baseline48$median[1], 0, baseline48$median[1], col = "azure4")
#segments(loffset, baseline32$median[1], 0, baseline32$median[1], col = "azure4")


cx <- 1
points(c(loffset), c(baseline96$median[1]), pch = 15, col = "darkblue", cex = cx)
points(c(loffset), c(baseline64$median[1]), pch = 8, col = "darkgreen", cex = cx)
points(c(loffset), c(baseline48$median[1]), pch = 17, col = "darkred", cex = cx)
points(c(loffset), c(baseline32$median[1]), pch = 18, cex = cx)
invisible(dev.off())

extras <- process_df(read.csv("data/e1/96k_extras.csv"), 0.05)

pdf("../paper/img/sparse_scaling_2.pdf", width = 4.5, height = 3)
par(mar=c(3,3,1, 0.6))
plot(ws96_data$cores, ws96_data$median, xlab='', ylab = '', cex = pcex, pch = 18, 
	 log="yx", axes = F, cex.lab = lcex, xlim = c(1, 1440), ylim = c(1, 4000), type = "n")

grid()
points(ws96_data$cores, ws96_data$median, cex = pcex, pch = 18)
points(extras$cores, extras$median, cex = pcex, pch = 18)

abline(NULL, NULL, h = baseline96$median[1], lty = 4)
legend('bottomleft', legend = c(expression(italic(ks)~baseline)), 
	   inset = 0.02, lty = c(4), lwd = c(1), col = c("black"), cex = lcex, bg = "white")

lines(ws96_data$cores, rep(ws96_data$median[1] *1, length(ws96_data$cores)) / ws96_data$cores)

title(xlab = "Number of processors", ylab = "Running time [s]", cex.lab = lcex, line = 1.3)
crs <- c(1, 36, 72, 144, 288, 720, 1440)
axis(side=1, at = crs, lab = crs, cex.axis = acex, tck = -0.03, mgp=c(3, .3, 0)); 
box()
axis(side=2, cex.axis = acex, tck = -0.03, mgp=c(3, .5, 0))

invisible(dev.off())


weak_scaling <- read.csv("data/e6/data.csv")
w32 <- process_df(weak_scaling[grepl("32k", weak_scaling$input), ], 0.05)
w48 <- process_df(weak_scaling[grepl("48k", weak_scaling$input), ], 0.05)
w64 <- process_df(weak_scaling[grepl("64k", weak_scaling$input), ], 0.05)
w80 <- process_df(weak_scaling[grepl("80k", weak_scaling$input), ], 0.05)
w96 <- process_df(weak_scaling[grepl("96k", weak_scaling$input), ], 0.05)

weak_times <- c(w32$median[1], w48$median[1], w64$median[1], w80$median[1], w96$median[1])
weak_sizes <- c(32000, 48000, 64000, 80000, 96000)

pdf("../paper/img/sparse_scaling_3.pdf", width = 4.7, height = 3)
par(mar=c(3,3,1, 0.6))
# 720 cores :)
plot(weak_sizes, weak_times, 
	 xlab = "Input size", ylab = "Running time [s]", cex = pcex, pch = 18, axes = F, cex.lab = 1.3, type = "n")
grid()  

points(weak_sizes, weak_times, cex = pcex, pch = 18)
abline(lm(weak_times~weak_sizes), lty = 2, untf=TRUE)
title(xlab = "Number of vertices (n)", ylab = "Running time [s]", cex.lab = lcex, line = 1.3)

axis(side=1, at = weak_sizes, lab = weak_sizes, cex.axis = acex, tck = -0.03, mgp=c(3, .3, 0)); 
box()
axis(side=2, cex.axis = acex, tck = -0.03, mgp=c(3, .5, 0))

legend('bottomright', legend = c("Linear trend (slope = 0.99)"), inset = 0.02, lty = c(2), lwd = c(1), col = c("black"), cex = lcex, bg = "white")
invisible(dev.off())


