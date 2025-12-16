#include "base/base_inc.h"
#include "os/os_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"

typedef struct {
  u32 a, b;
  u64 dist_sq;
} Edge;

typedef struct {
  Lane_Ctx lane_ctx;
  Arena *arena;
  String input;
  Vec3_s32 *points;
  u64 point_count;
  Edge *edges;
  u64 edge_count;
  u64 *edge_idx_counter;
  void *edge_temp;
  u64 *lane_histograms;
  u64 *global_histogram;
  u32 *uf_parent;
  u32 *uf_rank;
  u32 *uf_sizes;
  u64 *result_p1_slow;
  u64 *result_p1_fast;
  u64 *result_p2;
  u64 *time_p1_slow;
  u64 *time_p1_fast;
  u64 *time_p2;
} Thread_Params;

static u32 uf_find(u32 *parent, u32 i) {
  if (parent[i] != i) {
    parent[i] = uf_find(parent, parent[i]);
  }
  return parent[i];
}

static u32 uf_find_read_only(u32 *parent, u32 i) {
  while (parent[i] != i) {
    i = parent[i];
  }
  return i;
}

static void uf_union(u32 *parent, u32 *rank, u32 a, u32 b) {
  u32 ra = uf_find(parent, a);
  u32 rb = uf_find(parent, b);
  if (ra == rb)
    return;

  if (rank[ra] < rank[rb]) {
    parent[ra] = rb;
  } else if (rank[ra] > rank[rb]) {
    parent[rb] = ra;
  } else {
    parent[rb] = ra;
    rank[ra]++;
  }
}

static u64 edge_get_key(void *element) {
  Edge *e = (Edge *)element;
  return e->dist_sq;
}

static u64 parse_points(String input, Vec3_s32 *points, u64 max_count) {
  u64 count = 0;
  u8 *ptr = input.str;
  u8 *end = input.str + input.size;

  while (ptr < end && count < max_count) {
    while (ptr < end && !char_is_digit(*ptr) && *ptr != '-') ptr++;
    if (ptr >= end) break;

    s32 x = 0, y = 0, z = 0;
    s32 sign = 1;
    if (*ptr == '-') {
      sign = -1;
      ptr++;
    }
    while (ptr < end && char_is_digit(*ptr)) {
      x = x * 10 + (*ptr++ - '0');
    }
    x *= sign;

    while (ptr < end && *ptr == ',') ptr++;
    sign = 1;
    if (*ptr == '-') {
      sign = -1;
      ptr++;
    }
    while (ptr < end && char_is_digit(*ptr)) {
      y = y * 10 + (*ptr++ - '0');
    }
    y *= sign;

    while (ptr < end && *ptr == ',') ptr++;
    sign = 1;
    if (*ptr == '-') {
      sign = -1;
      ptr++;
    }
    while (ptr < end && char_is_digit(*ptr)) {
      z = z * 10 + (*ptr++ - '0');
    }
    z *= sign;

    points[count++] = (Vec3_s32){x, y, z};

    while (ptr < end && *ptr != '\n') ptr++;
    if (ptr < end)
      ptr++;
  }

  return count;
}

static void generate_edges_lane(Vec3_s32 *points, u64 point_count, Edge *edges,
                                u64 *edge_idx_counter) {
  Rng1U64 range = lane_range(point_count);

  for (u64 i = range.min; i < range.max; i++) {
    for (u64 j = i + 1; j < point_count; j++) {
      s64 dx = points[i].x - points[j].x;
      s64 dy = points[i].y - points[j].y;
      s64 dz = points[i].z - points[j].z;

      u64 idx = ins_atomic_u64_inc_eval(edge_idx_counter) - 1;
      edges[idx].a = (u32)i;
      edges[idx].b = (u32)j;
      edges[idx].dist_sq = (u64)(dx * dx + dy * dy + dz * dz);
    }
  }
}

static u64 solve_part1_lane(Edge *edges, u64 edge_count, u64 point_count,
                            u64 connections, u32 *parent, u32 *rank,
                            u32 *sizes) {
  Rng1U64 init_range = lane_range(point_count);
  for (u64 i = init_range.min; i < init_range.max; i++) {
    parent[i] = (u32)i;
    rank[i] = 0;
    sizes[i] = 0;
  }
  lane_sync();

  if (lane_idx() == 0) {
    u64 to_process = (connections < edge_count) ? connections : edge_count;
    for (u64 i = 0; i < to_process; i++) {
      uf_union(parent, rank, edges[i].a, edges[i].b);
    }
  }
  lane_sync();

  for (u64 i = init_range.min; i < init_range.max; i++) {
    u32 root = uf_find_read_only(parent, (u32)i);
    ins_atomic_u32_inc_eval(&sizes[root]);
  }
  lane_sync();

  u32 top3[3] = {0, 0, 0};
  if (lane_idx() == 0) {
    for (u64 i = 0; i < point_count; i++) {
      if (sizes[i] > top3[0]) {
        top3[2] = top3[1];
        top3[1] = top3[0];
        top3[0] = sizes[i];
      } else if (sizes[i] > top3[1]) {
        top3[2] = top3[1];
        top3[1] = sizes[i];
      } else if (sizes[i] > top3[2]) {
        top3[2] = sizes[i];
      }
    }
  }

  return (u64)top3[0] * (u64)top3[1] * (u64)top3[2];
}

static u64 solve_part2_lane(Vec3_s32 *points, Edge *edges, u64 edge_count,
                            u64 point_count, u32 *parent, u32 *rank) {
  Rng1U64 init_range = lane_range(point_count);
  for (u64 i = init_range.min; i < init_range.max; i++) {
    parent[i] = (u32)i;
    rank[i] = 0;
  }
  lane_sync();

  u64 last_a = 0, last_b = 0;
  if (lane_idx() == 0) {
    u64 unions_made = 0;
    for (u64 i = 0; i < edge_count && unions_made < point_count - 1; i++) {
      u32 ra = uf_find(parent, edges[i].a);
      u32 rb = uf_find(parent, edges[i].b);
      if (ra != rb) {
        uf_union(parent, rank, edges[i].a, edges[i].b);
        last_a = edges[i].a;
        last_b = edges[i].b;
        unions_made++;
      }
    }
  }

  lane_sync_u64(&last_a, 0);
  lane_sync_u64(&last_b, 0);

  return (u64)points[last_a].x * (u64)points[last_b].x;
}

static void thread_entry_point(void *p) {
  Thread_Params *params = (Thread_Params *)p;
  Lane_Ctx ctx = params->lane_ctx;

  TCTX *tctx = tctx_alloc();
  tctx_select(tctx);
  tctx_lane_init(ctx.lane_idx, ctx.lane_count, ctx.barrier,
                 ctx.broadcast_memory);

  Scratch scratch = scratch_begin(0, 0);

  generate_edges_lane(params->points, params->point_count, params->edges,
                      params->edge_idx_counter);
  lane_sync();

  u64 start, elapsed;
  u64 result;

  lane_sync();
  start = os_now_microseconds();
  radix_sort_u64_lane(params->edges, params->edge_count, sizeof(Edge),
                      edge_get_key, params->edge_temp, params->lane_histograms,
                      params->global_histogram);
  result = solve_part1_lane(params->edges, params->edge_count,
                            params->point_count, 1000, params->uf_parent,
                            params->uf_rank, params->uf_sizes);
  lane_sync();
  elapsed = os_now_microseconds() - start;
  if (lane_idx() == 0) {
    *params->result_p1_slow = result;
    *params->time_p1_slow = elapsed;
  }

  lane_sync();
  start = os_now_microseconds();
  result = solve_part1_lane(params->edges, params->edge_count,
                            params->point_count, 1000, params->uf_parent,
                            params->uf_rank, params->uf_sizes);
  lane_sync();
  elapsed = os_now_microseconds() - start;
  if (lane_idx() == 0) {
    *params->result_p1_fast = result;
    *params->time_p1_fast = elapsed;
  }

  lane_sync();
  start = os_now_microseconds();
  result =
      solve_part2_lane(params->points, params->edges, params->edge_count,
                       params->point_count, params->uf_parent, params->uf_rank);
  lane_sync();
  elapsed = os_now_microseconds() - start;
  if (lane_idx() == 0) {
    *params->result_p2 = result;
    *params->time_p2 = elapsed;
  }

  scratch_end(scratch);
  tctx_release(tctx);
}

void entry_point(Cmd_Line *cmd_line) {
  Arena *arena = arena_alloc();
  log_init(arena, str_lit(""));

  String input_path = str_lit("inputs/day_08.txt");
  String input = os_data_from_file_path(arena, input_path);
  if (input.size == 0) {
    print("Error: Could not read {S}\n", input_path);
    return;
  }

  u64 line_count = 0;
  for (u64 i = 0; i < input.size; i++) {
    if (input.str[i] == '\n')
      line_count++;
  }

  Vec3_s32 *points = push_array(arena, Vec3_s32, line_count);
  u64 point_count = parse_points(input, points, line_count);
  u64 edge_count = (point_count * (point_count - 1)) / 2;

  Edge *edges = push_array(arena, Edge, edge_count);
  void *edge_temp = push_array(arena, u8, edge_count * sizeof(Edge));
  u64 edge_idx_counter = 0;

  u64 num_lanes = os_get_system_info()->logical_processors;
  u64 *lane_histograms = push_array(arena, u64, num_lanes * 256);
  u64 *global_histogram = push_array(arena, u64, 256);

  u32 *uf_parent = push_array(arena, u32, point_count);
  u32 *uf_rank = push_array(arena, u32, point_count);
  u32 *uf_sizes = push_array(arena, u32, point_count);

  Barrier barrier = barrier_alloc(num_lanes);
  u64 broadcast_val = 0;

  Thread *threads = push_array(arena, Thread, num_lanes);
  Thread_Params *params = push_array(arena, Thread_Params, num_lanes);

  u64 result_p1_slow = 0, result_p1_fast = 0, result_p2 = 0;
  u64 time_p1_slow = 0, time_p1_fast = 0, time_p2 = 0;

  for (u64 i = 0; i < num_lanes; i++) {
    params[i].lane_ctx.lane_idx = i;
    params[i].lane_ctx.lane_count = num_lanes;
    params[i].lane_ctx.barrier = barrier;
    params[i].lane_ctx.broadcast_memory = &broadcast_val;
    params[i].arena = arena;
    params[i].input = input;
    params[i].points = points;
    params[i].point_count = point_count;
    params[i].edges = edges;
    params[i].edge_count = edge_count;
    params[i].edge_idx_counter = &edge_idx_counter;
    params[i].edge_temp = edge_temp;
    params[i].lane_histograms = lane_histograms;
    params[i].global_histogram = global_histogram;
    params[i].uf_parent = uf_parent;
    params[i].uf_rank = uf_rank;
    params[i].uf_sizes = uf_sizes;
    params[i].result_p1_slow = &result_p1_slow;
    params[i].result_p1_fast = &result_p1_fast;
    params[i].result_p2 = &result_p2;
    params[i].time_p1_slow = &time_p1_slow;
    params[i].time_p1_fast = &time_p1_fast;
    params[i].time_p2 = &time_p2;
    threads[i] = thread_launch(thread_entry_point, &params[i]);
  }

  for (u64 i = 0; i < num_lanes; i++) {
    thread_join(threads[i], MAX_U64); // I am not sure I like this thread_join function? 
  }

  print("=== Day 8 ===\n", num_lanes);

  char buf[32];
  u32 len;

  len = fmt_u64_to_str(result_p1_slow, buf, 10);
  buf[len] = 0;
  print("Part 1 (radix+lane): {s} (time: {u} us)\n", buf, (u32)time_p1_slow);

  len = fmt_u64_to_str(result_p1_fast, buf, 10);
  buf[len] = 0;
  print("Part 1 (lane only):  {s} (time: {u} us)\n", buf, (u32)time_p1_fast);

  print("\n");

  len = fmt_u64_to_str(result_p2, buf, 10);
  buf[len] = 0;
  print("Part 2 (lane):       {s} (time: {u} us)\n", buf, (u32)time_p2);

  barrier_release(barrier);
  arena_release(arena);
}
