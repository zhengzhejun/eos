/**
 *  @file
 *  @copyright defined in eosio/LICENSE.txt
 */
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/block_log.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/reversible_block_object.hpp>

#include <fc/io/json.hpp>
#include <fc/filesystem.hpp>
#include <fc/variant.hpp>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

using namespace eosio::chain;
namespace bfs = boost::filesystem;
namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;

struct blocklog
{
      blocklog()
      {
      }

      void read_log();
      void set_program_options(options_description &cli);
      void initialize(const variables_map &options);

      bfs::path blocks_dir;
      bfs::path eos_output_file;
      bfs::path prochaintech_output_file;
      uint32_t first_block;
      uint32_t last_block;
      bool no_pretty_print;
      bool as_json_array;
      // abi_def eosio_abi_def;
      abi_def eosio_token_abi_def;
      abi_def prochaintech_abi_def;
};

void blocklog::read_log()
{
      block_log block_logger(blocks_dir);
      const auto end = block_logger.read_head();
      EOS_ASSERT(end, block_log_exception, "No blocks found in block log");
      EOS_ASSERT(end->block_num() > 1, block_log_exception, "Only one block found in block log");

      ilog("existing block log contains block num 1 through block num ${n}", ("n", end->block_num()));

      //    optional<chainbase::database> reversible_blocks;
      //    try {
      //       reversible_blocks.emplace(blocks_dir / config::reversible_blocks_dir_name, chainbase::database::read_only, config::default_reversible_cache_size);
      //       reversible_blocks->add_index<reversible_block_index>();
      //       const auto& idx = reversible_blocks->get_index<reversible_block_index,by_num>();
      //       auto first = idx.lower_bound(end->block_num());
      //       auto last = idx.rbegin();
      //       if (first != idx.end() && last != idx.rend())
      //          ilog( "existing reversible block num ${first} through block num ${last} ", ("first",first->get_block()->block_num())("last",last->get_block()->block_num()) );
      //       else {
      //          elog( "no blocks available in reversible block database: only block_log blocks are available" );
      //          reversible_blocks.reset();
      //       }
      //    } catch( const std::runtime_error& e ) {
      //       if( std::string(e.what()) == "database dirty flag set" ) {
      //          elog( "database dirty flag set (likely due to unclean shutdown): only block_log blocks are available" );
      //       } else if( std::string(e.what()) == "database metadata dirty flag set" ) {
      //          elog( "database metadata dirty flag set (likely due to unclean shutdown): only block_log blocks are available" );
      //       } else {
      //          throw;
      //       }
      //    }

      std::ofstream eos_output_blocks;
      std::ostream *eos_out;
      if (!eos_output_file.empty())
      {
            eos_output_blocks.open(eos_output_file.generic_string().c_str());
            if (eos_output_blocks.fail())
            {
                  std::ostringstream ss;
                  ss << "Unable to open file '" << eos_output_file.string() << "'";
                  throw std::runtime_error(ss.str());
            }
            eos_out = &eos_output_blocks;
      }
      else
            eos_out = &std::cout;

      std::ofstream prochaintech_output_blocks;
      std::ostream *prochaintech_out;
      if (!prochaintech_output_file.empty())
      {
            prochaintech_output_blocks.open(prochaintech_output_file.generic_string().c_str());
            if (prochaintech_output_blocks.fail())
            {
                  std::ostringstream ss;
                  ss << "Unable to open file '" << prochaintech_output_file.string() << "'";
                  throw std::runtime_error(ss.str());
            }
            prochaintech_out = &prochaintech_output_blocks;
      }
      else
            prochaintech_out = &std::cout;

      uint32_t block_num = (first_block < 1) ? 1 : first_block;
      signed_block_ptr next;
      fc::variant pretty_output;
      const fc::microseconds deadline = fc::seconds(10);
      auto print_block = [&](signed_block_ptr &next) {
            // abi_serializer::to_variant(*next,
            //                            pretty_output,
            //                            [](account_name n) { return optional<abi_serializer>(); },
            //                            deadline);
            // const auto block_id = next->id();
            // const uint32_t ref_block_prefix = block_id._hash[1];
            for (const transaction_receipt &receipt : next->transactions)
            {
                  if (receipt.trx.contains<packed_transaction>())
                  {
                        const packed_transaction &pt = receipt.trx.get<packed_transaction>();
                        const auto &raw = pt.get_raw_transaction();
                        const auto &trx = fc::raw::unpack<transaction>(raw);
                        bool isEos = false;
                        bool isProchaintech = false;
                        vector<fc::variant> action_variants;
                        for (const action &act : trx.actions)
                        {
                              if (act.account == N(eosio) && act.name == N(setabi))
                              {
                                    eosio::chain::setabi setabi_act = act.data_as<eosio::chain::setabi>();
                                    if (setabi_act.account == N(eosio.token))
                                    {
                                          std::cout << "success get eosio.token set abi" << std::endl;
                                          eosio_token_abi_def = fc::raw::unpack<abi_def>( setabi_act.abi );
                                    }
                                    if (setabi_act.account == N(prochaintech)) 
                                    {
                                          std::cout << "success get prochaintech set abi" << std::endl;
                                          prochaintech_abi_def = fc::raw::unpack<abi_def>(setabi_act.abi);
                                    }
                              }
                              // if (act.account == N(eosio.token))
                              // {
                              //       isEos = true;
                              //       fc::variant action_variant;
                              //       abi_serializer::to_variant(act, action_variant, [&](account_name n) {
                              //             optional<abi_serializer> result = abi_serializer(eosio_token_abi_def, deadline);
                              //             return result;
                              //       }, deadline);
                              //       action_variants.push_back(action_variant);
                              // }
                              if (act.account == N(prochaintech))
                              {
                                    isProchaintech = true;
                                    fc::variant action_variant;
                                    abi_serializer::to_variant(act, action_variant, [&](account_name n) {
                                          optional<abi_serializer> result = abi_serializer(prochaintech_abi_def, deadline);
                                          return result;
                                    }, deadline);
                                    action_variants.push_back(action_variant);
                              }
                        }
                        if (isEos)
                        {
                              fc::mutable_variant_object unpacked_transaction_variant(trx);
                              unpacked_transaction_variant.set("id", trx.id().str());
                              unpacked_transaction_variant.set("actions", action_variants);
                              *eos_out << fc::json::to_pretty_string(unpacked_transaction_variant) << "\n";
                        }
                        if (isProchaintech)
                        {
                              fc::mutable_variant_object unpacked_transaction_variant(trx);
                              unpacked_transaction_variant.set("id", trx.id().str());
                              unpacked_transaction_variant.set("actions", action_variants);
                              *prochaintech_out << fc::json::to_pretty_string(unpacked_transaction_variant) << "\n";
                        }
                  }
            }

            // const auto enhanced_object = fc::mutable_variant_object
            //            ("block_num",next->block_num())
            //            ("id", block_id)
            //            ("ref_block_prefix", ref_block_prefix)
            //            (pretty_output.get_object());
            // fc::variant v(std::move(enhanced_object));
            //  if (no_pretty_print)
            //     fc::json::to_stream(*out, v, fc::json::stringify_large_ints_and_doubles);
            //  else
            //     *out << fc::json::to_pretty_string(v) << "\n";
      };
      bool contains_obj = false;
      while ((block_num <= last_block) && (next = block_logger.read_block_by_num(block_num)))
      {
            print_block(next);
            ++block_num;
            if (block_num % 1000 == 0)
            {
                  std::cout << block_num << " : " << last_block<< std::endl;
            }
            contains_obj = true;
      }
}

void blocklog::set_program_options(options_description &cli)
{
      cli.add_options()("blocks-dir", bpo::value<bfs::path>()->default_value("blocks"), "the location of the blocks directory (absolute path or relative to the current directory)")("prochaintech-output-file", bpo::value<bfs::path>(), "the file to write the block log output to (absolute or relative path).  If not specified then output is to stdout.")("eos-output-file", bpo::value<bfs::path>(), "the file to write the block log output to (absolute or relative path).  If not specified then output is to stdout.")("first", bpo::value<uint32_t>(&first_block)->default_value(1), "the first block number to log")("last", bpo::value<uint32_t>(&last_block)->default_value(std::numeric_limits<uint32_t>::max()), "the last block number (inclusive) to log")("no-pretty-print", bpo::bool_switch(&no_pretty_print)->default_value(false), "Do not pretty print the output.  Useful if piping to jq to improve performance.")("as-json-array", bpo::bool_switch(&as_json_array)->default_value(false), "Print out json blocks wrapped in json array (otherwise the output is free-standing json objects).")("help", "Print this help message and exit.");
}

void blocklog::initialize(const variables_map &options)
{
      try
      {
            auto bld = options.at("blocks-dir").as<bfs::path>();
            if (bld.is_relative())
                  blocks_dir = bfs::current_path() / bld;
            else
                  blocks_dir = bld;

            if (options.count("eos-output-file"))
            {
                  bld = options.at("eos-output-file").as<bfs::path>();
                  if (bld.is_relative())
                        eos_output_file = bfs::current_path() / bld;
                  else
                        eos_output_file = bld;
            }
            if (options.count("prochaintech-output-file"))
            {
                  bld = options.at("prochaintech-output-file").as<bfs::path>();
                  if (bld.is_relative())
                        prochaintech_output_file = bfs::current_path() / bld;
                  else
                        prochaintech_output_file = bld;
            }
            // eosio_abi_def = fc::json::from_file(options.at("eosio-abi-path").as<std::string>()).as<abi_def>();
      }
      FC_LOG_AND_RETHROW()
}

int main(int argc, char **argv)
{
      std::ios::sync_with_stdio(false); // for potential performance boost for large block log files
      options_description cli("eosio-blocklog command line options");
      try
      {
            blocklog blog;
            blog.set_program_options(cli);
            variables_map vmap;
            bpo::store(bpo::parse_command_line(argc, argv, cli), vmap);
            bpo::notify(vmap);
            if (vmap.count("help") > 0)
            {
                  cli.print(std::cerr);
                  return 0;
            }
            blog.initialize(vmap);
            blog.read_log();
      }
      catch (const fc::exception &e)
      {
            elog("${e}", ("e", e.to_detail_string()));
            return -1;
      }
      catch (const boost::exception &e)
      {
            elog("${e}", ("e", boost::diagnostic_information(e)));
            return -1;
      }
      catch (const std::exception &e)
      {
            elog("${e}", ("e", e.what()));
            return -1;
      }
      catch (...)
      {
            elog("unknown exception");
            return -1;
      }

      return 0;
}
