plot_run_stats = function(file_prefix)
{
	file_name = paste("output_cplex/",file_prefix, "_run_stats.csv", sep="")
	data = read.csv(file_name, head=TRUE)
	methods = c('Primal', 'Dual', 'Barrier')
	for (method in methods)
	{
		method_data = data[data$Method == method,]
		plot_file = paste("cplex_plots/run_stats/",file_prefix, "_", method, "_run_stats.jpg", sep="")
		jpeg(plot_file)
		plot(log(method_data[,6]), log(((method_data[,2]+method_data[,3])^2)*sqrt(method_data[,4])), col="blue", ylab="log((r+c)^2 * sqrt(N))", xlab="log(Time)", main=method)
		dev.off()
	}
}

plot_itn_obj_stats = function(file_prefix)
{
	file_name = paste("output_cplex/", file_prefix, "_itn_obj_stats.csv", sep="")
	data = read.csv(file_name, head=TRUE)
	methods = c('Primal', 'Dual')#, 'Barrier')
	for (method in methods)
	{
		method_data = data[data$Method == method,]
		tapply(1:nrow(method_data), as.factor(method_data[,1]), function(r){
									jpeg(paste("cplex_plots/itn_obj/",method_data[r[1],1],"_", method, "_itn_obj.jpg", sep=""))
									plot(method_data[r,2], method_data[r,3],
										col="blue", ylab="Objective Value", xlab="Iterations", 
										main=paste(method_data[r[1],1],"_", method))
									dev.off()
								})
	}
}

plot_itn_eps_stats = function(file_prefix)
{
	file_name = paste("output_cplex/", file_prefix, "_itn_obj_stats.csv", sep="")
	data = read.csv(file_name, head=TRUE)
	methods = c('Primal', 'Dual')#, 'Barrier')
	for (method in methods)
	{
		method_data = data[data$Method == method,]
		tapply(1:nrow(method_data), as.factor(method_data[,1]), function(r){
									jpeg(paste("cplex_plots/itn_eps/",method_data[r[1],1],"_", method, "_itn_obj.jpg", sep=""))
									plot(method_data[r,2], 
										abs(method_data[r,3] - method_data[r[length(r)],3])/method_data[r[length(r)],3],
										col="blue", ylab="Objective Value", xlab="Iterations", 
										main=paste(method_data[r[1],1],"_", method))
									dev.off()
								})
	}
}

plot_itn_time_stats = function(file_prefix)
{
	file_name = paste("output_cplex/", file_prefix, "_itn_time_stats.csv", sep="")
	data = read.csv(file_name, head=TRUE)
	data = data[data$Time >= 0,]
#	print(data)
	methods = c('Barrier')
	for (method in methods)
	{
		method_data = data[data$Method == method,]
		tapply(1:nrow(method_data), as.factor(method_data[,1]), function(r){
									if (length(r) >= 5)
									{
										jpeg(paste("cplex_plots/itn_time/",method_data[r[1],1],"_", method, "_itn_time.jpg", sep=""))
										plot(method_data[r,2], method_data[r,3], col="blue", xlab="Time", 										ylab="Iterations", main=method_data[r[1],1])
										dev.off()
									}
								})
	}
}
