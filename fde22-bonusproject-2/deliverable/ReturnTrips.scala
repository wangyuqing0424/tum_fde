import org.apache.spark.sql.SparkSession
import org.apache.spark.SparkContext
import org.apache.spark.SparkContext._
import org.apache.spark.SparkConf
import org.apache.spark.sql.types._
import org.apache.spark.sql.functions._
import org.apache.spark.sql.Column
import org.apache.spark.sql.Dataset
import org.apache.spark.sql.Row

// (c) 2021 Thomas Neumann, Timo Kersten, Alexander Beischl, Maximilian Reif

object ReturnTrips {
  def compute(trips : Dataset[Row], dist : Double, spark : SparkSession) : Dataset[Row] = {

    import spark.implicits._

    //    ToDo: Add your implementation here
    val diff_time = 28800
    val interval = dist * 0.001 / 111.2
    val earthRadius = 6371

    def makeDistExpr(lat1: Double, lon1: Double, lat2: Double, lon2: Double) = {
      val dLat = (lat2 - lat1).toRadians
      val dLon = (lon2 - lon1).toRadians
      val hav = Math.pow(Math.sin(dLat*0.5), 2) + Math.pow(Math.sin(dLon*0.5), 2) * Math.cos(lat1.toRadians) * Math.cos(lat2.toRadians)
      earthRadius * 2 * Math.asin(Math.sqrt(hav)) * 1000
    }
    val udf_makeDistExpr = udf(makeDistExpr _)



    val trips_process =trips
      .select(
        "pickup_longitude",
        "pickup_latitude",
        "dropoff_longitude",
        "dropoff_latitude",
        "tpep_pickup_datetime",
        "tpep_dropoff_datetime")
      .withColumn("tpep_pickup_datetime",unix_timestamp($"tpep_pickup_datetime"))
      .withColumn("tpep_dropoff_datetime",unix_timestamp($"tpep_dropoff_datetime"))

    val trips_lat_bucket = trips_process
      .withColumn("new_dropoff_datetime", floor($"tpep_dropoff_datetime" / diff_time))
      .withColumn("bucket_pickup_latitude",
        explode(
          array(
            floor($"pickup_latitude" /interval ) - 1,
            floor($"pickup_latitude" / interval),
            floor($"pickup_latitude" / interval) + 1
          )
        )
      )
      .withColumn(
        "bucket_dropoff_latitude",
        explode(
          array(
            floor($"dropoff_latitude" / interval) - 1,
            floor($"dropoff_latitude" / interval),
            floor($"dropoff_latitude" / interval) + 1
          )
        )
      )

    val trips_time_bucket = trips_process.withColumn(
      "bucket_pickup_time",
      explode(
        array(
          floor($"tpep_pickup_datetime" / diff_time) - 1,
          floor($"tpep_pickup_datetime" / diff_time)
        )))
      .withColumn(
        "new_dropoff_latitude",
        floor($"dropoff_latitude" / interval)
      )
      .withColumn(
        "new_pickup_latitude",
        floor($"pickup_latitude" / interval)
      )


    val tripsselect= trips_lat_bucket.as("a")
      .join(trips_time_bucket.as("b"),
        $"a.new_dropoff_datetime" === $"b.bucket_pickup_time" &&
          $"a.bucket_pickup_latitude" === $"b.new_dropoff_latitude" &&
          $"b.new_pickup_latitude" === $"a.bucket_dropoff_latitude",
      )
      .filter(
        $"a.tpep_dropoff_datetime" < $"b.tpep_pickup_datetime" &&
          $"a.tpep_dropoff_datetime" + diff_time > $"b.tpep_pickup_datetime" &&
          (udf_makeDistExpr($"a.dropoff_latitude",$"a.dropoff_longitude",$"b.pickup_latitude",$"b.pickup_longitude") < lit(dist))&&
          (udf_makeDistExpr($"b.dropoff_latitude",$"b.dropoff_longitude",$"a.pickup_latitude",$"a.pickup_longitude") < lit(dist)))
    tripsselect
  }
}
