#' Given a sorted list of n observations, this function
#' returns the ranks of elements that denote the 1-p confidence 
#' interval of the median
lb_cirank <- function(n, p) {
  n = as.double(n); p = as.double(p)
  lo = floor((n - qnorm(1 - p/2) * sqrt(n))/2)
  hi = floor(1 + (n + qnorm(1 - p/2) * sqrt(n))/2)
  if (lo < 1 || hi > n) {
    stop(paste("Too few samples (", n, ") to guarantee the given CI"))
  }
  return (c(lo, hi))
}

#' Draws error bars into an existing plot
#'
#' @param x Vector of x axis locations
#' @param low_ci Vector of CI lower bounds corresponding to the x locations (same length)
#' @param hi_ci Vector of CI upper bounds corresponding to the x locations (same length)
#' @param epsilon Width of the whiskers
error_bars <- function(x, means, low_ci, hi_ci, epsilon, col = "black", lwd = 1) {
  segments(x, low_ci, x, hi_ci, col = col, lwd = lwd)
  segments(x - epsilon, hi_ci, x + epsilon, hi_ci, col = col, lwd = lwd)
  segments(x - epsilon, low_ci, x + epsilon, low_ci, col = col, lwd = lwd)
}

extract_ci_lo <- function(df, core_count, ci_value) {
  observations <- df[df$cores == core_count, ]$time
  return (sort(observations)[lb_cirank(length(observations), ci_value)[1]])
}

extract_ci_hi <- function(df, core_count, ci_value) {
  observations <- df[df$cores == core_count, ]$time
  return (sort(observations)[lb_cirank(length(observations), ci_value)[2]])
}

process_df <- function(df, ci_value, override_ci = F) {
  cores <- unique(df$cores)
  
  median <- sapply(cores, function(core_count) {
    median(df[df$cores == core_count, ]$time)
  })
  
  
  observations <- sapply(cores, function(core_count) {
    length(df[df$cores == core_count, ]$time)
  })
  
  if (override_ci) {
    ci_lo <- rep(0, length(cores))
    ci_hi <- rep(0, length(cores))
    ci_mean_ratio <- rep(0, length(cores))
  } else {
    ci_lo <- sapply(cores, function(core_count) {
      extract_ci_lo(df, core_count, ci_value) 
    })
    
    ci_hi <- sapply(cores, function(core_count) {
      extract_ci_hi(df, core_count, ci_value) 
    })
    
    ci_mean_ratio <- sapply(cores, function(core_count) {
      observations <- df[df$cores == core_count, ]$time
      (extract_ci_hi(df, core_count, ci_value) - extract_ci_lo(df, core_count, ci_value)) / mean(observations)
    })
  }
  
  new_df <- data.frame(cores, median, observations, ci_lo, ci_hi, ci_mean_ratio)
  return (new_df[order(new_df$cores), ]);
}
