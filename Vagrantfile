Vagrant::Config.run do |config|

  config.vm.box = "lucid32"
  config.vm.box_url = "http://files.vagrantup.com/lucid32.box"

  config.vm.network("33.33.33.90")

  config.vm.provision :chef_solo do |chef|

    chef.recipe_url = "http://cloud.github.com/downloads/d0ugal/chef_recipes/cookbooks.tar.gz"
    chef.cookbooks_path = [:vm, "cookbooks"]

    chef.add_recipe "main"

  end

end
