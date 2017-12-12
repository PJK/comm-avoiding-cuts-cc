source("R/common.R")

rmat_data <- read.csv("data/e4/data.csv")

rmat1k_data <- process_df(rmat_data[grepl("_1024", rmat_data$input), ], 0.05)
rmat2k_data <- process_df(rmat_data[grepl("_2048", rmat_data$input), ], 0.05)
rmat4k_data <- process_df(rmat_data[grepl("_4096", rmat_data$input), ], 0.05)


options(scipen=5)
lcex <- 0.7
pcex <- 1.2
acex <- 0.7


baseline <- read.csv("data/e4/baselines.csv")
baseline1k<- process_df(baseline[grepl("_1024", baseline$input), ], 0.05)
baseline2k <- process_df(baseline[grepl("_2048", baseline$input), ], 0.05)
baseline4k <- process_df(baseline[grepl("_4096", baseline$input), ], 0.05)


plotClass <- function(d1, d2, d3, limits = c(5, 50)) {
  par(mar=c(3,3,1, 0.6))
  plot(d1$cores, d1$median,  xlab='', ylab = '', xlim = c(0, 1296), cex = pcex, 
	   pch = 18, ylim = limits, axes = F, cex.lab = lcex, type = "n")
  grid()
  
  
  loffset <- 0
  lend <- 72
  segments(loffset, baseline4k$median[1], 1296, baseline4k$median[1], col = "azure4")
  segments(loffset, baseline2k$median[1], 1296, baseline2k$median[1], col = "azure4")
  segments(loffset, baseline1k$median[1], 1296, baseline1k$median[1], col = "azure4")
  
  points(d1$cores, d1$median, cex = pcex, pch = 18)
  
  title(xlab = "Number of processors", ylab = "Running time [s]", cex.lab = lcex, line = 1.3)
  
  lines(d1$cores[2:3], d1$median[2:3], lty = 3, lwd = 2)
  lines(d1$cores[4:6], d1$median[4:6], lty = 3, lwd = 2)
  lines(d1$cores[7:10], d1$median[7:10], lty = 3, lwd = 2)
  
  points(d2$cores, d2$median, cex = pcex, pch = 17, col = "darkred")
  lines(d2$cores[2:3], d2$median[2:3], col = "darkred", lty = 2)
  lines(d2$cores[4:6], d2$median[4:6], col = "darkred", lty = 2)
  lines(d2$cores[7:10], d2$median[7:10], col = "darkred", lty = 2)
  
  points(d3$cores, d3$median, cex = pcex, pch = 8, col = "darkgreen")
  lines(d3$cores[2:3], d3$median[2:3],lty = 2, col = "darkgreen")
  lines(d3$cores[4:7], d3$median[4:7], lty = 2, col = "darkgreen")
  lines(d3$cores[8:9], d3$median[8:9], lty = 2, col = "darkgreen")
  
  axis(side=1, at = d1$cores, lab = d1$cores, cex.axis = acex, tck = -0.03, mgp=c(3, .3, 0)); box()
  axis(side=2, cex.axis =acex, tck = -0.03, mgp=c(3, .5, 0))
  
  box()

  legend('topright', legend = c("m = 4'096n", "m = 2'048n", "m = 1'024n"), 
		 inset = 0.01, pch = c(8, 17, 18),
		 bty = "y",
		 col = c("darkgreen", "darkred", "black"), cex = lcex, bg = "white")
  

  

  cx <- 1
  points(c(loffset), c(baseline4k$median[1]), pch = 8, col = "darkgreen", cex = cx)
  points(c(loffset), c(baseline2k$median[1]), pch = 17, col = "darkred", cex = cx)
  points(c(loffset), c(baseline1k$median[1]), pch = 18, cex = cx)
  
}

pdf("../paper/img/dense_scaling_1.pdf", width = 4.5, height = 3)
plotClass(rmat1k_data, rmat2k_data, rmat4k_data)
invisible(dev.off())

print(c(baseline1k$median[1], baseline2k$median[1], baseline4k$median[1]))
