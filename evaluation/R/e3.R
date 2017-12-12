source("R/common.R")

raw256_data <- read.csv("data/e3/256k.csv")
ba256_data <- process_df(raw256_data[grepl("ba", raw256_data$input), ], 0.05)
ws256_data <- process_df(raw256_data[grepl("ws", raw256_data$input), ], 0.05)

raw8M_data <- read.csv("data/e3/8M.csv")
ba8M_data <- process_df(raw8M_data[grepl("ba", raw8M_data$input), ], 0.05)
ws8M_data <- process_df(raw8M_data[grepl("ws", raw8M_data$input), ], 0.05)

raw16M_data <- read.csv("data/e3/16M.csv")
ba16M_data <- process_df(raw16M_data[grepl("ba", raw16M_data$input), ], 0.05)
ws16M_data <- process_df(raw16M_data[grepl("ws", raw16M_data$input), ], 0.05)

plotClass <- function(d1, d2, d3, limits = NULL) {
  par(mar=c(4,4,1, 1))
  plot(d1$cores, d1$median, xlab = "Number of processors", ylab = "Running time [s]", 
       cex = 1.5, pch = 18, ylim = limits, axes = F, cex.lab = 1.3, log = "y")
  lines(d1$cores, d1$median,lty = 2, col = "darkred")
  
  points(d2$cores, d2$median, cex = 1.5, pch = 17, col = "darkred")
  lines(d2$cores, d2$median,lty = 2, col = "darkred")
  
  points(d3$cores, d3$median, cex = 1.5, pch = 8, col = "darkgreen")
  lines(d3$cores, d3$median,lty = 2, col = "darkgreen")

  axis(side=1, at = d1$cores, lab = d1$cores, cex.axis = 1); box()
  axis(side=2)
  
  box()
  grid()  
}


plotClass(ba256_data, ba8M_data, ba16M_data, limits=c(0.2,  30))
plotClass(ws256_data, ws8M_data, ws16M_data, limits=c(0.2,  30))

baseline32 <- process_df(baseline[grepl("ws_32", baseline$input), ], 0.05)
baseline48 <- process_df(baseline[grepl("ws_48", baseline$input), ], 0.05)
baseline64 <- process_df(baseline[grepl("ws_64", baseline$input), ], 0.05)
baseline96 <- process_df(baseline[grepl("ws_96", baseline$input), ], 0.05)
